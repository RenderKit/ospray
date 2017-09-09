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
      Light();
      Light(const std::string &type);

      virtual void preCommit(RenderContext &ctx) override;
      virtual void postCommit(RenderContext &ctx) override;

      //! \brief returns a std::string with the c++ name of this class
      virtual std::string toString() const override;

    protected:
      /*! \brief light type, i.e., 'DirectionalLight', 'PointLight', ... */
      std::string type = "none";
    };

    struct OSPSG_INTERFACE AmbientLight : public Light
    {
      AmbientLight();
    };

    struct OSPSG_INTERFACE DirectionalLight : public Light
    {
      DirectionalLight();
    };

    struct OSPSG_INTERFACE PointLight : public Light
    {
      PointLight();
    };

    struct OSPSG_INTERFACE QuadLight : public Light
    {
      QuadLight();
    };

    struct OSPSG_INTERFACE HDRILight : public Light
    {
      HDRILight();
      virtual bool computeValid() override;
      virtual void postCommit(RenderContext &ctx) override;
    };


  } // ::ospray::sg
} // ::ospray
