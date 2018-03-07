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

#include "Data.h"
// core ospray
#include "ospray/common/OSPCommon.h"

namespace ospray {
  namespace sg {

    std::string DataBuffer::arrayTypeAsString() const
    {
      return stringForType(type);
    }

    OSP_REGISTER_SG_NODE(DataVector1uc);
    OSP_REGISTER_SG_NODE(DataVector1f);
    OSP_REGISTER_SG_NODE(DataVector2f);
    OSP_REGISTER_SG_NODE(DataVector3f);
    OSP_REGISTER_SG_NODE(DataVector3fa);
    OSP_REGISTER_SG_NODE(DataVector4f);
    OSP_REGISTER_SG_NODE(DataVector1i);
    OSP_REGISTER_SG_NODE(DataVector2i);
    OSP_REGISTER_SG_NODE(DataVector3i);
    OSP_REGISTER_SG_NODE(DataVector4i);
    OSP_REGISTER_SG_NODE(DataVectorOSP);
    OSP_REGISTER_SG_NODE(DataVectorRAW);
    OSP_REGISTER_SG_NODE(DataVectorAffine3f);

  } // ::ospray::sg
} // ::ospray
