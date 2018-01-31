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

#include "NodeParameter.h"

namespace ospray {
  namespace sg {

    //OSPRay types
    OSP_REGISTER_SG_NODE_NAME(NodeParam<float>, float);
    OSP_REGISTER_SG_NODE_NAME(NodeParam<int>, int);
    OSP_REGISTER_SG_NODE_NAME(NodeParam<bool>, bool);
    OSP_REGISTER_SG_NODE_NAME(NodeParam<vec2f>, vec2f);
    OSP_REGISTER_SG_NODE_NAME(NodeParam<vec2i>, vec2i);
    OSP_REGISTER_SG_NODE_NAME(NodeParam<vec3f>, vec3f);
    OSP_REGISTER_SG_NODE_NAME(NodeParam<vec3f>, vec3i);
    OSP_REGISTER_SG_NODE_NAME(NodeParam<vec3f>, vec3fa);
    OSP_REGISTER_SG_NODE_NAME(NodeParam<vec3f>, vec4f);
    OSP_REGISTER_SG_NODE_NAME(NodeParam<box3f>, box2f);
    OSP_REGISTER_SG_NODE_NAME(NodeParam<box3f>, box2i);
    OSP_REGISTER_SG_NODE_NAME(NodeParam<box3f>, box3f);
    OSP_REGISTER_SG_NODE_NAME(NodeParam<box3f>, box3i);
    OSP_REGISTER_SG_NODE_NAME(NodeParam<std::string>, string);
    OSP_REGISTER_SG_NODE_NAME(NodeParam<OSPObject>, OSPObject);

  } // ::ospray::sg
} // ::ospray
