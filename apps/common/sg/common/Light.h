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

#include "sg/common/Node.h"
#include "sg/common/Serialization.h"
#include "sg/camera/Camera.h"

#include <cfloat>

namespace ospray {
  namespace sg {

    /*! a light node - the generic light node */
    struct OSPSG_INTERFACE Light : public sg::Node
    {
      //! \brief constructor
      Light() = default;
      Light(const std::string &type) : type(type) {}

      virtual void preCommit(RenderContext &ctx)
      {
        if (!ospLight)
          ospLight = ospNewLight(ctx.ospRenderer, type.c_str());
        ospCommit(ospLight);
        setValue((OSPObject)ospLight);
      }

      virtual void postCommit(RenderContext &ctx)
      {
        ospCommit(ospLight);
      }

      //! \brief returns a std::string with the c++ name of this class
      virtual std::string toString() const override { return "ospray::sg::Light"; }

      /*! \brief light type, i.e., 'DirectionalLight', 'PointLight', ... */
      const std::string type = "none";
      OSPLight ospLight {nullptr};
    };

    /*! a light node - the generic light node */
    struct OSPSG_INTERFACE AmbientLight : public Light
    {
      //! \brief constructor
      AmbientLight() : Light("AmbientLight") {}

      virtual void init() override;
    };

    /*! a light node - the generic light node */
    struct OSPSG_INTERFACE DirectionalLight : public Light
    {
      //! \brief constructor
      DirectionalLight() : Light("DirectionalLight") {}

      virtual void init() override;
    };

    /*! a light node - the generic light node */
    struct OSPSG_INTERFACE PointLight : public Light
    {
      //! \brief constructor
      PointLight() : Light("PointLight") {}

      virtual void init() override;
    };

  } // ::ospray::sg
} // ::ospray
