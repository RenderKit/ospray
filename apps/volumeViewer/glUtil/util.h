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

#pragma once

#include "ospray/ospray.h"

#ifdef _WIN32
#  ifdef ospray_module_opengl_EXPORTS
#    define OSPGLUTIL_INTERFACE __declspec(dllexport)
#  else
#    define OSPGLUTIL_INTERFACE __declspec(dllimport)
#  endif
#else
#  define OSPGLUTIL_INTERFACE
#endif

namespace ospray {
  namespace opengl {

    /*! \brief Compute and return an OSPRay depth texture from the current OpenGL context,
        assuming a perspective projection.

      This function automatically determines the parameters of the OpenGL perspective
      projection and camera direction / up vectors. It then reads the OpenGL depth
      buffer and transforms it to an OSPRay depth texture where the depth values
      represent ray distances from the camera.
    */
    OSPGLUTIL_INTERFACE OSPTexture2D getOSPDepthTextureFromOpenGLPerspective();

    /*! \brief Compute and return an OSPRay depth texture from the provided view parameters
         and OpenGL depth buffer, assuming a perspective projection.

      This function transforms the provided OpenGL depth buffer to an OSPRay depth texture
      where the depth values represent ray distances from the camera.

      \param fovy Specifies the field of view angle, in degrees, in the y direction

      \param aspect Specifies the aspect ratio that determines the field of view in the x
      direction

      \param zNear,zFar Specifies the distances from the viewer to the near and far
      clipping planes

      \param cameraDir,cameraUp The camera direction and up vectors

      \param glDepthBuffer The OpenGL depth buffer, can be read via glReadPixels() using
      the GL_FLOAT format. The application is responsible for freeing this buffer.

      \param glDepthBufferWidth,glDepthBufferHeight Dimensions of the provided OpenGL depth
      buffer
    */
    OSPGLUTIL_INTERFACE OSPTexture2D getOSPDepthTextureFromOpenGLPerspective(const double &fovy,
                                                                const double &aspect,
                                                                const double &zNear,
                                                                const double &zFar,
                                                                const osp::vec3f &cameraDir,
                                                                const osp::vec3f &cameraUp,
                                                                const float *glDepthBuffer,
                                                                const size_t &glDepthBufferWidth,
                                                                const size_t &glDepthBufferHeight);

    /*! \brief Compute and return OpenGL depth values from the depth component of the given
        OSPRay framebuffer, using parameters of the current OpenGL context and assuming a
        perspective projection.

        This function automatically determines the parameters of the OpenGL perspective
        projection and camera direction / up vectors. It assumes these values match those
        provided to OSPRay (fovy, aspect, camera direction / up vectors). It then maps the
        OSPRay depth buffer and transforms it to OpenGL depth values according to the OpenGL
        perspective projection.

        The OSPRay frame buffer object must have been constructed with the OSP_FB_DEPTH flag.
    */
    OSPGLUTIL_INTERFACE float * getOpenGLDepthFromOSPPerspective(OSPFrameBuffer frameBuffer,
                                                    const osp::vec2i &frameBufferSize);

    /*! \brief Compute and return OpenGL depth values from the provided view parameters and
        OSPRay depth buffer, assuming a perspective projection.

        \param fovy Specifies the field of view angle, in degrees, in the y direction

        \param aspect Specifies the aspect ratio that determines the field of view in the x
        direction

        \param zNear,zFar Specifies the distances from the viewer to the near and far
        clipping planes

        \param cameraDir,cameraUp The camera direction and up vectors

        \param ospDepthBuffer The OSPRay depth buffer, can be read via
        ospMapFrameBuffer(frameBuffer, OSP_FB_DEPTH). The application is responsible for
        freeing this buffer.

        \param frameBufferSize Dimensions of the provided OSPRay depth uffer
    */
    OSPGLUTIL_INTERFACE float * getOpenGLDepthFromOSPPerspective(const double &fovy,
                                                    const double &aspect,
                                                    const double &zNear,
                                                    const double &zFar,
                                                    const osp::vec3f &cameraDir,
                                                    const osp::vec3f &cameraUp,
                                                    const float *ospDepthBuffer,
                                                    const osp::vec2i &frameBufferSize);

  }
}
