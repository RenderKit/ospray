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

#include "Builder.h"
#include "ospray_testing.h"

using namespace ospcommon::math;

namespace ospray {
  namespace testing {

    struct PtLuminous : public detail::Builder
    {
      PtLuminous()           = default;
      ~PtLuminous() override = default;

      cpp::Group buildGroup() const override;
      cpp::World buildWorld() const override;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    cpp::Group PtLuminous::buildGroup() const
    {
      cpp::Geometry sphereGeometry("spheres");

      sphereGeometry.setParam("sphere.position", cpp::Data(vec3f(0.f)));
      sphereGeometry.setParam("radius", 1.f);
      sphereGeometry.commit();

      cpp::GeometricModel model(sphereGeometry);

      cpp::Material material(rendererType, "Luminous");
      material.setParam("color", vec3f(0.7f, 0.7f, 1.f));
      material.commit();

      model.setParam("material", cpp::Data(material));
      model.commit();

      cpp::Group group;

      group.setParam("geometry", cpp::Data(model));
      group.commit();

      return group;
    }

    cpp::World PtLuminous::buildWorld() const
    {
      auto world = Builder::buildWorld();
      world.removeParam("light");
      return world;
    }

    OSP_REGISTER_TESTING_BUILDER(PtLuminous, test_pt_luminous);

  }  // namespace testing
}  // namespace ospray
