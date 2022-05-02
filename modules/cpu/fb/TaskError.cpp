// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "TaskError.h"
#include <vector>

namespace ospray {

TaskError::TaskError(const vec2i &_numTasks)
    : numTasks(_numTasks), taskErrorBuffer(numTasks.long_product())
{
  // maximum number of regions: all regions are of size 3 are split in half
  errorRegion.reserve(divRoundUp(taskErrorBuffer.size() * 2, size_t(3)));
  clear();
}

void TaskError::clear()
{
  std::fill(taskErrorBuffer.begin(), taskErrorBuffer.end(), inf);

  errorRegion.clear();
  // initially create one region covering the complete tile/image
  errorRegion.push_back(box2i(vec2i(0), numTasks));
}

float TaskError::operator[](const int id) const
{
  if (taskErrorBuffer.empty()) {
    return inf;
  }

  return taskErrorBuffer[id];
}

void TaskError::update(const vec2i &task, const float err)
{
  if (!taskErrorBuffer.empty()) {
    taskErrorBuffer[task.y * numTasks.x + task.x] = err;
  }
}

float TaskError::refine(const float errorThreshold)
{
  if (taskErrorBuffer.empty()) {
    return inf;
  }

  float maxErr = 0.f;
  float sumActErr = 0.f;
  int activeTasks = 0;
  for (const auto &err : taskErrorBuffer) {
    maxErr = std::max(maxErr, err);
    if (err > errorThreshold) {
      sumActErr += err;
      activeTasks++;
    }
  }
  const float error = activeTasks ? sumActErr / activeTasks : maxErr;

  // process regions first, but don't process newly split regions again
  int regions = errorThreshold > 0.f ? errorRegion.size() : 0;
  for (int i = 0; i < regions; i++) {
    box2i &region = errorRegion[i];
    float err = 0.f;
    float maxErr = 0.f;
    for (int y = region.lower.y; y < region.upper.y; y++)
      for (int x = region.lower.x; x < region.upper.x; x++) {
        int idx = y * numTasks.x + x;
        err += taskErrorBuffer[idx];
        maxErr = std::max(maxErr, taskErrorBuffer[idx]);
      }
    if (maxErr > errorThreshold) {
      // set all tasks of this region to >errorThreshold to enforce their
      // refinement as a group
      const float minErr = nextafter(errorThreshold, inf);
      for (int y = region.lower.y; y < region.upper.y; y++)
        for (int x = region.lower.x; x < region.upper.x; x++) {
          int idx = y * numTasks.x + x;
          taskErrorBuffer[idx] = std::max(taskErrorBuffer[idx], minErr);
        }
    }
    const vec2i size = region.size();
    const int area = reduce_mul(size);
    err /= area; // == avg
    if (err <= 4.f * errorThreshold) { // split region?
      // if would just contain single task after split or wholly done: remove
      if (area <= 2 || maxErr <= errorThreshold) {
        regions--;
        errorRegion[i] = errorRegion[regions];
        errorRegion[regions] = errorRegion.back();
        errorRegion.pop_back();
        i--;
        continue;
      }
      const vec2i split = region.lower + size / 2; // TODO: find split with
                                                   //       equal variance
      errorRegion.push_back(region); // region ref might become invalid
      if (size.x > size.y) {
        errorRegion[i].upper.x = split.x;
        errorRegion.back().lower.x = split.x;
      } else {
        errorRegion[i].upper.y = split.y;
        errorRegion.back().lower.y = split.y;
      }
    }
  }
  return error;
}

float *TaskError::errorBuffer()
{
  if (taskErrorBuffer.empty()) {
    return nullptr;
  }
  return taskErrorBuffer.data();
}

} // namespace ospray
