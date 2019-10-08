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

#pragma once

#include "ospray/ospray_cpp.h"
#include "ospray/ospray_util.h"
// ospcommon
#include "ospcommon/math/box.h"
#include "ospcommon/math/vec.h"

#include "ospray_testing_export.h"

extern "C" {

using osp_vec2f = ospcommon::math::vec2f;
using osp_vec3f = ospcommon::math::vec3f;
using osp_box3f = ospcommon::math::box3f;

typedef struct
{
  OSPData auxData;
  OSPGeometry geometry;
  OSPGeometricModel model;
  OSPGroup group;
  OSPInstance instance;
  osp_box3f bounds;
} OSPTestingGeometry;

typedef struct
{
  OSPVolume volume;
  osp_box3f bounds;
  osp_vec2f voxelRange;
} OSPTestingVolume;

/* Create an OSPRay renderer with sensible defaults for testing */
OSPRAY_TESTING_EXPORT
OSPRenderer ospTestingNewRenderer(const char *type OSP_DEFAULT_VAL("scivis"));

/* Create an OSPRay geometry (from a registered name), with the given renderer
 * type to create materials */
OSPRAY_TESTING_EXPORT
OSPTestingGeometry ospTestingNewGeometry(
    const char *geom_type, const char *renderer_type OSP_DEFAULT_VAL("scivis"));

/* Create an OSPRay geometry (from a registered name) */
OSPRAY_TESTING_EXPORT
OSPTestingVolume ospTestingNewVolume(const char *volume_type);

/* Create an OSPRay geometry (from a registered name) */
OSPRAY_TESTING_EXPORT
OSPTransferFunction ospTestingNewTransferFunction(
    osp_vec2f voxelRange, const char *tf_name OSP_DEFAULT_VAL("grayscale"));

/* Create an OSPRay perspective camera which looks at the center of the given
 * bounding box
 *
 * NOTE: this only sets 'dir', 'pos', and 'up'
 */
OSPRAY_TESTING_EXPORT
OSPCamera ospTestingNewDefaultCamera(osp_box3f bounds);

/* Create a list of lights, using a given preset name */
OSPRAY_TESTING_EXPORT
OSPData ospTestingNewLights(
    const char *lighting_set_name OSP_DEFAULT_VAL("ambient_only"));

}  // extern "C"

//////////////////////////////////////////////////////////////////////////////
// New C++ interface /////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

namespace ospray {
  namespace testing {

    using namespace ospcommon;
    using namespace ospcommon::math;

    using SceneBuilderHandle = void *;

    OSPRAY_TESTING_EXPORT
    SceneBuilderHandle newBuilder(const std::string &type);

    template <typename T>
    void setParam(SceneBuilderHandle b, const std::string &type, const T &val);

    OSPRAY_TESTING_EXPORT
    cpp::Group buildGroup(SceneBuilderHandle b);

    OSPRAY_TESTING_EXPORT
    cpp::World buildWorld(SceneBuilderHandle b);

    OSPRAY_TESTING_EXPORT
    void commit(SceneBuilderHandle b);

    OSPRAY_TESTING_EXPORT
    void release(SceneBuilderHandle b);

  }  // namespace testing
}  // namespace ospray

#include "detail/ospray_testing.inl"
