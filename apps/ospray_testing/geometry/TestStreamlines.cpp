// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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

#include "Geometry.h"
// ospcommon
#include "ospcommon/box.h"

using namespace ospcommon;

namespace ospray {
  namespace testing {

    struct TestStreamlines : public Geometry
    {
      TestStreamlines()           = default;
      ~TestStreamlines() override = default;

      OSPTestingGeometry createGeometry(
          const std::string &renderer_type) const override;
    };

    OSPTestingGeometry TestStreamlines::createGeometry(
        const std::string &renderer_type) const
    {
      auto slGeom  = ospNewGeometry("streamlines");
      auto slModel = ospNewGeometricModel(slGeom);

      OSPGroup group = ospNewGroup();
      auto models    = ospNewData(1, OSP_OBJECT, &slModel);
      ospSetData(group, "geometries", models);
      ospCommit(group);
      ospRelease(models);

      box3f bounds = empty;

      OSPMaterial slMat = ospNewMaterial(renderer_type.c_str(), "OBJMaterial");
      ospCommit(slMat);

      OSPInstance instance = ospNewInstance(group);
      ospCommit(instance);

      OSPTestingGeometry retval;
      retval.geometry = slGeom;
      retval.model    = slModel;
      retval.group    = group;
      retval.instance = instance;
      retval.bounds   = reinterpret_cast<osp_box3f &>(bounds);

      return retval;
    }

  }  // namespace testing
}  // namespace ospray
