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

// ospray_testing
#include "ospray_testing.h"
// ospcommon
#include "ospcommon/box.h"

using namespace ospcommon;

extern "C" OSPRenderer ospTestingNewRenderer(const char *type)
{
  auto renderer = ospNewRenderer(type);
  return renderer;
}

extern "C" OSPTestingGeometry ospTestingNewGeometry(const char *geom_type,
                                                    const char *renderer_type)
{
  // TODO
  return {};
}

extern "C" OSPTestingVolume ospTestingNewVolume(const char *geom_type)
{
  // TODO
  return {};
}

extern "C" OSPCamera ospTestingNewDefaultCamera(osp::box3f _bounds)
{
  auto camera = ospNewCamera("perspective");

  auto &upper = _bounds.upper;
  auto &lower = _bounds.lower;
  box3f bounds(vec3f(upper.x, upper.y, upper.z),
               vec3f(lower.x, lower.y, lower.z));
  vec3f diag = bounds.size();
  diag       = max(diag, vec3f(0.3f * length(diag)));

  auto gaze = ospcommon::center(bounds);
  auto pos  = gaze - .75f * vec3f(-.6 * diag.x, -1.2f * diag.y, .8f * diag.z);
  auto up   = vec3f(0.f, 1.f, 0.f);
  auto dir  = normalize(gaze - pos);

  ospSet3f(camera, "pos", pos.x, pos.y, pos.z);
  ospSet3f(camera, "dir", dir.x, dir.y, dir.z);
  ospSet3f(camera, "up", up.x, up.y, up.z);
  ospCommit(camera);

  return camera;
}

extern "C" OSPData ospTestingNewLights(const char *lighting_set_name)
{
  // TODO
  return nullptr;
}
