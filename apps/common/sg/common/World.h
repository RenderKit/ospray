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

#include "Renderable.h"
#include "Serialization.h"
#include "../camera/Camera.h"

namespace ospray {
  namespace sg {

    struct OSPSG_INTERFACE Model : public Renderable
    {
      Model();
      virtual ~Model() override = default;
      virtual std::string toString() const override;

      //commit caches renders.  It will render children during commit, and add
      //cached rendered children during render call.
      virtual void traverse(RenderContext &ctx,
                            const std::string& operation) override;
      virtual void preCommit(RenderContext &ctx) override;
      virtual void postCommit(RenderContext &ctx) override;

      OSPModel ospModel();

    protected:

      OSPModel stashedModel{nullptr};
    };

    /*! a world node */
    struct OSPSG_INTERFACE World : public Model
    {
      World() = default;
      virtual ~World() override = default;

      /*! \brief returns a std::string with the c++ name of this class */
      virtual std::string toString() const override;

      virtual void preCommit(RenderContext &ctx) override;
      virtual void postCommit(RenderContext &ctx) override;

    protected:

      std::shared_ptr<sg::World> stashedWorld;
    };


    struct OSPSG_INTERFACE Instance : public World
    {
      Instance();
      ~Instance() override = default;

      /*! \brief return bounding box in world coordinates.

      This function can be used by the viewer(s) for calibrating
      camera motion, setting default camera position, etc. Nodes
      for which that does not apply can simpy return
      box3f(embree::empty) */
      box3f bounds() const override;

      //Instance caches renders.  It will render children during commit, and add
         //cached rendered children during render call.
      void traverse(RenderContext &ctx, const std::string& operation) override;
      void preCommit(RenderContext &ctx) override;
      void postCommit(RenderContext &ctx) override;
      void preRender(RenderContext &ctx) override;
      void postRender(RenderContext &ctx) override;

    private:

      OSPGeometry ospInstance {nullptr};
      //currently, nested instances do not appear to work in OSPRay.  To get around this,
      // instanced can be manually turned off for parent instancegroups.
      bool instanced {true};
      ospcommon::affine3f baseTransform{ospcommon::one};

      void updateInstance(RenderContext &ctx);
      void updateTransform(RenderContext &ctx);
      bool instanceDirty{true};
      ospcommon::affine3f cachedTransform{ospcommon::one};
      ospcommon::affine3f worldTransform{ospcommon::one};
      //    computed from baseTransform*position*rotation*scale
      ospcommon::affine3f oldTransform{ospcommon::one};

    };

  } // ::ospray::sg
} // ::ospray


