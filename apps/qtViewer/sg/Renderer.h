// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

#include "SceneGraph.h"

namespace ospray {
  namespace sg {
    struct Renderer : public RefCount {
      Renderer();

      /*! re-start accumulation (for progressive rendering). make sure
          that this function gets called at lesat once every time that
          anything changes that might change the appearance of the
          converged image (e.g., camera position, scene, frame size,
          etc) */
      void resetAccumulation();

      void setWorld(const Ref<sg::World> &world);
      void setCamera(const Ref<sg::Camera> &camera);
      void setIntegrator(const Ref<sg::Integrator> &integrator);

      // -------------------------------------------------------
      // query functions
      // -------------------------------------------------------
      
      //! find the last camera in the scene graph
      sg::Camera *getLastDefinedCamera() const;
      //! find the last integrator in the scene graph
      sg::Integrator *getLastDefinedIntegrator() const;
      
      //! create a default camera
      Ref<sg::Camera> createDefaultCamera(vec3f up=vec3f(0,1,0));

      // //! set a default camera
      // void setDefaultCamera() { setCamera(createDefaultCamera()); }

      /*! render a frame. return 0 if successful, any non-zero number if not */
      virtual int renderFrame();

      // =======================================================
      // state variables
      // =======================================================
      Ref<sg::World>       world;
      Ref<sg::Camera>      camera;
      Ref<sg::FrameBuffer> frameBuffer;
      Ref<sg::Integrator>  integrator;
      // Ref<Frame>  frame;

      // state variables
      /*! all _unique_ nodes (i.e, even instanced nodes are listed
          only once */
      Serialization uniqueNodes;
      /*! _all_ nodes (i.e, instanced nodes are listed once for each
          time they are instanced */
      Serialization allNodes;

      //! accumulation ID
      size_t accumID;
    };
  }
}

