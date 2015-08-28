// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
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

#ifdef __APPLE__
  #include <OpenGL/gl.h>
#else
  #include <GL/gl.h>
#endif

namespace ospray {
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

      const double fovy = 2. * atan(1.0f / m5) * 180./M_PI;
      const double aspect = m5 / m0;
      const double zNear = (m14 * (1.0f - k)) / (2.0f * k);
      const double zFar = k * zNear;

      // get camera direction and up vectors from model view matrix
      GLdouble glModelViewMatrix[16];
      glGetDoublev(GL_MODELVIEW_MATRIX, glModelViewMatrix);

      const osp::vec3f  cameraUp( glModelViewMatrix[1],  glModelViewMatrix[5],  glModelViewMatrix[9]);
      const osp::vec3f cameraDir(-glModelViewMatrix[2], -glModelViewMatrix[6], -glModelViewMatrix[10]);

      // get window dimensions from OpenGL viewport
      GLint glViewport[4];
      glGetIntegerv(GL_VIEWPORT, glViewport);

      const size_t width = glViewport[2];
      const size_t height = glViewport[3];

      // read OpenGL depth buffer
      float *glDepthBuffer = new float[width * height];
      glReadPixels(0, 0, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, (GLvoid *)glDepthBuffer);

      // get an OSPRay depth texture from the OpenGL depth buffer
      OSPTexture2D depthTexture = getOSPDepthTextureFromOpenGLPerspective(fovy, aspect, zNear, zFar, cameraDir, cameraUp, glDepthBuffer, width, height);
                                                                   
      // free allocated depth buffer
      delete[] glDepthBuffer;

      // return OSPRay depth texture
      return depthTexture;
    }

    OSPTexture2D getOSPDepthTextureFromOpenGLPerspective(double fovy,
                                                         double aspect,
                                                         double zNear,
                                                         double zFar,
                                                         osp::vec3f cameraDir,
                                                         osp::vec3f cameraUp,
                                                         float *glDepthBuffer,
                                                         size_t glDepthBufferWidth,
                                                         size_t glDepthBufferHeight)
    {
      // this should later be done in ISPC...

      // transform OpenGL depth to linear depth
      for (size_t i=0; i<glDepthBufferWidth*glDepthBufferHeight; i++) {
        const double z_n = 2.0 * glDepthBuffer[i] - 1.0;
        glDepthBuffer[i] = 2.0 * zNear * zFar / (zFar + zNear - z_n * (zFar - zNear));
      }

      // transform from orthogonal Z depth to ray distance t
      osp::vec3f dir_du = normalize(cross(cameraDir, cameraUp));
      osp::vec3f dir_dv = normalize(cross(dir_du, cameraDir));

      const float imagePlaneSizeY = 2.f * tanf(fovy/2.f * M_PI/180.f);
      const float imagePlaneSizeX = imagePlaneSizeY * aspect;

      dir_du *= imagePlaneSizeX;
      dir_dv *= imagePlaneSizeY;

      const osp::vec3f dir_00 = cameraDir - .5f * dir_du - .5f * dir_dv;

      for (size_t j=0; j<glDepthBufferHeight; j++)
        for (size_t i=0; i<glDepthBufferWidth; i++) {
          osp::vec3f dir_ij = normalize(dir_00 + float(i)/float(glDepthBufferWidth-1) * dir_du + float(j)/float(glDepthBufferHeight-1) * dir_dv);

          float t = glDepthBuffer[j*glDepthBufferWidth+i] / dot(cameraDir, dir_ij);
          glDepthBuffer[j*glDepthBufferWidth+i] = t;
        }

      // nearest texture filtering required for depth textures -- we don't want interpolation of depth values...
      OSPTexture2D depthTexture = ospNewTexture2D(glDepthBufferWidth, glDepthBufferHeight, OSP_FLOAT, glDepthBuffer, OSP_TEXTURE_FILTER_NEAREST);

      return depthTexture;
    }

  }
}
