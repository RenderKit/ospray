// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

// ospray
#include "SimpleAO.h"
#include "SimpleAOMaterial.h"
// ispc exports
#include "SimpleAO_ispc.h"

namespace ospray {

  //! \brief Constructor
  SimpleAO::SimpleAO(int defaultNumSamples) : numSamples(defaultNumSamples)
  {
    setParam<std::string>("externalNameFromAPI", "ao");

    ispcEquivalent = ispc::SimpleAO_create(this);
  }

  /*! \brief common function to help printf-debugging */
  std::string SimpleAO::toString() const
  {
    return "ospray::render::SimpleAO";
  }

  /*! \brief commit the object's outstanding changes
   *         (such as changed parameters etc) */
  void SimpleAO::commit()
  {
    Renderer::commit();

    int   numSamples = getParam1i("aoSamples", 1);
    float rayLength  = getParam1f("aoDistance", 1e20f);
    ispc::SimpleAO_set(getIE(), numSamples, rayLength);
  }

  OSP_REGISTER_RENDERER(SimpleAO(4), ao);

  OSP_REGISTER_RENDERER(SimpleAO(1),  ao1);
  OSP_REGISTER_RENDERER(SimpleAO(2),  ao2);
  OSP_REGISTER_RENDERER(SimpleAO(4),  ao4);
  OSP_REGISTER_RENDERER(SimpleAO(8),  ao8);
  OSP_REGISTER_RENDERER(SimpleAO(16), ao16);

} // ::ospray

