// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <vector>
#include "common/ISPCRTBuffers.h"
#include "common/OSPCommon.h"
#include "rkcommon/containers/AlignedVector.h"

namespace ospray {

/* manages error per tile and adaptive regions, for variance estimation
  Implementation based on Dammertz et al., "A Hierarchical Automatic Stopping
  Condition for Monte Carlo Global Illumination", WSCG 2010 */
class OSPRAY_SDK_INTERFACE TaskError
{
 public:
  TaskError(ispcrt::Context &ispcrtContext, const vec2i &numTasks);

  // The default constructor will make an empty task error region
  TaskError() = default;

  void clear();

  float operator[](const int id) const;

  void update(const vec2i &task, const float error);

  float refine(const float errorThreshold);

  // Return the error buffer, or null if there are no tiles that error is
  // tracked for
  float *errorBuffer();

 protected:
  vec2i numTasks = vec2i(0);
  // holds error per task
  std::unique_ptr<BufferShared<float>> taskErrorBuffer;
  // image regions (in #tasks) that do not yet estimate the error on
  // per-task basis
  std::vector<box2i> errorRegion;
};
} // namespace ospray
