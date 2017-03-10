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

#include "SceneGraph.h"

namespace ospray {
  namespace sg {

    class FrameBuffer;

    struct Renderer : public Renderable
    {
      Renderer() = default;
      virtual void init() override;
      virtual void preRender(RenderContext &ctx) override;
      virtual void postRender(RenderContext &ctx) override;

      /*! re-start accumulation (for progressive rendering). make sure
          that this function gets called at lesat once every time that
          anything changes that might change the appearance of the
          converged image (e.g., camera position, scene, frame size,
          etc) */
      void resetAccumulation();

      void setWorld(const std::shared_ptr<sg::World> &world);
      void setCamera(const std::shared_ptr<sg::Camera> &camera);
      void setIntegrator(const std::shared_ptr<sg::Integrator> &integrator);

      // -------------------------------------------------------
      // query functions
      // -------------------------------------------------------
      
      //! find the last camera in the scene graph
      std::shared_ptr<sg::Camera> lastDefinedCamera() const;

      //! find the last integrator in the scene graph
      std::shared_ptr<sg::Integrator> lastDefinedIntegrator() const;
      
      //! create a default camera
      std::shared_ptr<sg::Camera> createDefaultCamera(vec3f up = vec3f(0,1,0));

      /*! render a frame. return 0 if successful, any non-zero number if not */
      virtual int renderFrame();

      void preCommit(RenderContext &ctx) override;
      void postCommit(RenderContext &ctx) override;

      // =======================================================
      // state variables
      // =======================================================
      std::shared_ptr<sg::World>       world;
      std::shared_ptr<sg::Camera>      camera;
      std::shared_ptr<sg::FrameBuffer> frameBuffer;
      std::shared_ptr<sg::Integrator>  integrator;
      OSPRenderer ospRenderer {nullptr};
      OSPData lightsData={nullptr};
      TimeStamp lightsBuildTime;
      TimeStamp frameMTime;
      std::string createdType = "none";

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

  } // ::ospray::sg
} // ::ospray

