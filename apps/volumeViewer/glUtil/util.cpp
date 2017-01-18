// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "util.h"
#include "ospcommon/vec.h"

#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#endif

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#include <math.h>

namespace ospray {
  typedef ospcommon::vec2i vec2i;
  typedef ospcommon::vec3f vec3f;

  namespace opengl {

    OSPTexture2D getOSPDepthTextureFromOpenGLPerspective()
    {
      // compute fovy, aspect, zNear, zFar from OpenGL projection matrix
      GLdouble glProjectionMatrix[16];
      glGetDoublev(GL_PROJECTION_MATRIX, glProjectionMatrix);

      const double m0  = glProjectionMatrix[0];
      const double m5  = glProjectionMatrix[5];
      const double m10 = glProjectionMatrix[10];
      const double m14 = glProjectionMatrix[14];
      const double k = (m10 - 1.0f) / (m10 + 1.0f);

      const double fovy = 2. * atanf(1.0f / m5) * 180./M_PI;
      const double aspect = m5 / m0;
      const double zNear = (m14 * (1.0f - k)) / (2.0f * k);
      const double zFar = k * zNear;

      // get camera direction and up vectors from model view matrix
      GLdouble glModelViewMatrix[16];
      glGetDoublev(GL_MODELVIEW_MATRIX, glModelViewMatrix);

      const ospray::vec3f  cameraUp( glModelViewMatrix[1],  glModelViewMatrix[5],  glModelViewMatrix[9]);
      const ospray::vec3f cameraDir(-glModelViewMatrix[2], -glModelViewMatrix[6], -glModelViewMatrix[10]);

      // get window dimensions from OpenGL viewport
      GLint glViewport[4];
      glGetIntegerv(GL_VIEWPORT, glViewport);

      const size_t width = glViewport[2];
      const size_t height = glViewport[3];

      // read OpenGL depth buffer
      float *glDepthBuffer = new float[width * height];
      glReadPixels(0, 0, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, (GLvoid *)glDepthBuffer);

      // get an OSPRay depth texture from the OpenGL depth buffer
      OSPTexture2D depthTexture
        = getOSPDepthTextureFromOpenGLPerspective(fovy, aspect, zNear, zFar, 
                                                  (osp::vec3f&)cameraDir, (osp::vec3f&)cameraUp, 
                                                  glDepthBuffer, width, height);

      // free allocated depth buffer
      delete[] glDepthBuffer;

      // return OSPRay depth texture
      return depthTexture;
    }

    OSPTexture2D getOSPDepthTextureFromOpenGLPerspective(const double &fovy,
                                                         const double &aspect,
                                                         const double &zNear,
                                                         const double &zFar,
                                                         const osp::vec3f &_cameraDir,
                                                         const osp::vec3f &_cameraUp,
                                                         const float *glDepthBuffer,
                                                         const size_t &glDepthBufferWidth,
                                                         const size_t &glDepthBufferHeight)
    {
      ospray::vec3f cameraDir = (ospray::vec3f&)_cameraDir;
      ospray::vec3f cameraUp = (ospray::vec3f&)_cameraUp;
      // this should later be done in ISPC...

      float *ospDepth = new float[glDepthBufferWidth * glDepthBufferHeight];

      // transform OpenGL depth to linear depth
      for (size_t i=0; i<glDepthBufferWidth*glDepthBufferHeight; i++) {
        const double z_n = 2.0 * glDepthBuffer[i] - 1.0;
        ospDepth[i] = 2.0 * zNear * zFar / (zFar + zNear - z_n * (zFar - zNear));
      }
      
      // transform from orthogonal Z depth to ray distance t
      ospray::vec3f dir_du = normalize(cross(cameraDir, cameraUp));
      ospray::vec3f dir_dv = normalize(cross(dir_du, cameraDir));

      const float imagePlaneSizeY = 2.f * tanf(fovy/2.f * M_PI/180.f);
      const float imagePlaneSizeX = imagePlaneSizeY * aspect;

      dir_du *= imagePlaneSizeX;
      dir_dv *= imagePlaneSizeY;

      const ospray::vec3f dir_00 = cameraDir - .5f * dir_du - .5f * dir_dv;

      for (size_t j=0; j<glDepthBufferHeight; j++)
        for (size_t i=0; i<glDepthBufferWidth; i++) {
          const ospray::vec3f dir_ij = normalize(dir_00 + float(i)/float(glDepthBufferWidth-1) * dir_du + float(j)/float(glDepthBufferHeight-1) * dir_dv);

          const float t = ospDepth[j*glDepthBufferWidth+i] / dot(cameraDir, dir_ij);
          ospDepth[j*glDepthBufferWidth+i] = t;
        }

      // nearest texture filtering required for depth textures -- we don't want interpolation of depth values...
      vec2i texSize(glDepthBufferWidth, glDepthBufferHeight);
      OSPTexture2D depthTexture = ospNewTexture2D((osp::vec2i&)texSize, OSP_TEXTURE_R32F, ospDepth, OSP_TEXTURE_FILTER_NEAREST);

      delete[] ospDepth;

      return depthTexture;
    }

    float * getOpenGLDepthFromOSPPerspective(OSPFrameBuffer frameBuffer,
                                             const osp::vec2i &frameBufferSize)
    {
      // compute fovy, aspect, zNear, zFar from OpenGL projection matrix
      // this assumes fovy, aspect match the values set on the OSPRay perspective camera!
      GLdouble glProjectionMatrix[16];
      glGetDoublev(GL_PROJECTION_MATRIX, glProjectionMatrix);

      const double m0  = glProjectionMatrix[0];
      const double m5  = glProjectionMatrix[5];
      const double m10 = glProjectionMatrix[10];
      const double m14 = glProjectionMatrix[14];
      const double k = (m10 - 1.0f) / (m10 + 1.0f);

      const double fovy = 2. * atan(1.0f / m5) * 180./M_PI;
      const double aspect = m5 / m0;
      const double zNear = (m14 * (1.0f - k)) / (2.0f * k);
      const double zFar = k * zNear;

      // get camera direction and up vectors from model view matrix
      // again, this assumes these values match those set on the OSPRay camera!
      GLdouble glModelViewMatrix[16];
      glGetDoublev(GL_MODELVIEW_MATRIX, glModelViewMatrix);

      const ospray::vec3f  cameraUp( glModelViewMatrix[1],  glModelViewMatrix[5],  glModelViewMatrix[9]);
      const ospray::vec3f cameraDir(-glModelViewMatrix[2], -glModelViewMatrix[6], -glModelViewMatrix[10]);

      // map OSPRay depth buffer from provided frame buffer
      const float *ospDepthBuffer = (const float *)ospMapFrameBuffer(frameBuffer, OSP_FB_DEPTH);

      // get OpenGL depth from OSPRay depth
      float *glDepth
        = getOpenGLDepthFromOSPPerspective(fovy, aspect, zNear, zFar, 
                                           (osp::vec3f&)cameraDir, (osp::vec3f&)cameraUp, 
                                           ospDepthBuffer, frameBufferSize);

      // unmap OSPRay depth buffer
      ospUnmapFrameBuffer(ospDepthBuffer, frameBuffer);

      return glDepth;
    }


    float * getOpenGLDepthFromOSPPerspective(const double &fovy,
                                             const double &aspect,
                                             const double &zNear,
                                             const double &zFar,
                                             const osp::vec3f &_cameraDir,
                                             const osp::vec3f &_cameraUp,
                                             const float *ospDepthBuffer,
                                             const osp::vec2i &frameBufferSize)
    {
      ospray::vec3f cameraDir = (ospray::vec3f&)_cameraDir;
      ospray::vec3f cameraUp = (ospray::vec3f&)_cameraUp;
      // this should later be done in ISPC...
      
      const size_t ospDepthBufferWidth =  (size_t)frameBufferSize.x;
      const size_t ospDepthBufferHeight = (size_t)frameBufferSize.y;

      float *glDepth = new float[ospDepthBufferWidth * ospDepthBufferHeight];

      // transform from ray distance t to orthogonal Z depth
      ospray::vec3f dir_du = normalize(cross(cameraDir, cameraUp));
      ospray::vec3f dir_dv = normalize(cross(dir_du, cameraDir));

      const float imagePlaneSizeY = 2.f * tanf(fovy/2.f * M_PI/180.f);
      const float imagePlaneSizeX = imagePlaneSizeY * aspect;

      dir_du *= imagePlaneSizeX;
      dir_dv *= imagePlaneSizeY;

      const ospray::vec3f dir_00 = cameraDir - .5f * dir_du - .5f * dir_dv;

      for (size_t j=0; j<ospDepthBufferHeight; j++)
        for (size_t i=0; i<ospDepthBufferWidth; i++) {
          const ospray::vec3f dir_ij = normalize(dir_00 + float(i)/float(ospDepthBufferWidth-1) * dir_du + float(j)/float(ospDepthBufferHeight-1) * dir_dv);

          glDepth[j*ospDepthBufferWidth+i] = ospDepthBuffer[j*ospDepthBufferWidth+i] * dot(cameraDir, dir_ij);
        }

      // transform from linear to nonlinear OpenGL depth
      const double A = -(zFar + zNear) / (zFar - zNear);
      const double B = -2. * zFar*zNear / (zFar - zNear);

      for (size_t i=0; i<ospDepthBufferWidth*ospDepthBufferHeight; i++)
        glDepth[i] = 0.5*(-A*glDepth[i] + B) / glDepth[i] + 0.5;

      return glDepth;
    }

  }
}
