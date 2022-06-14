// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "LinearTransferFunction.h"
#include "volume/transferFunction/LinearTransferFunction_ispc.h"

namespace ospray {

LinearTransferFunction::LinearTransferFunction()
{
  getSh()->super.get = ispc::LinearTransferFunction_get_addr();
  getSh()->super.getMaxOpacity =
      ispc::LinearTransferFunction_getMaxOpacity_addr();
  getSh()->super.valueRange = range1f(0.0f, 1.0f);
}

void LinearTransferFunction::commit()
{
  TransferFunction::commit();

  colorValues = getParamDataT<vec3f>("color", true);
  opacityValues = getParamDataT<float>("opacity", true);

  getSh()->color = *ispc(colorValues);
  getSh()->opacity = *ispc(opacityValues);

  precomputeMaxOpacityRanges();
}

std::string LinearTransferFunction::toString() const
{
  return "ospray::LinearTransferFunction";
}

std::vector<range1i> LinearTransferFunction::getPositiveOpacityIndexRanges()
    const
{
  std::vector<range1i> intervals;

  range1i interval;
  bool rangeActive = false;

  const DataT<float> &opacities = *opacityValues;

  for (int i = 0; i < int(opacities.size()); i++) {
    if (opacities[i] > 0.f && !rangeActive) {
      rangeActive = true;
      interval.lower = i;
    } else if (opacities[i] <= 0.f && rangeActive) {
      rangeActive = false;
      interval.upper = i;
      intervals.push_back(interval);
    }
  }

  // special case for final value
  if (opacities[opacities.size() - 1] > 0.f) {
    if (rangeActive) {
      rangeActive = false;
      interval.upper = opacities.size();
      intervals.push_back(interval);
    } else {
      throw std::runtime_error("getPositiveOpacityIndexRanges() error");
    }
  }

  return intervals;
}

std::vector<range1f> LinearTransferFunction::getPositiveOpacityValueRanges()
    const
{
  std::vector<range1f> valueRanges;

  // determine index ranges for positive opacities
  std::vector<range1i> indexRanges = getPositiveOpacityIndexRanges();

  const DataT<float> &opacities = *opacityValues;

  // convert index ranges to value ranges
  // note that a positive opacity value has a span of +/-1 due to the linear
  // interpolation, and returned index ranges are [min, max) intervals
  for (int i = 0; i < int(indexRanges.size()); i++) {
    int minValueIndex = indexRanges[i].lower - 1;
    int maxValueIndex = indexRanges[i].upper;

    range1f range(neg_inf, inf);

    if (minValueIndex >= 0) {
      range.lower = valueRange.lower
          + minValueIndex * valueRange.size() / (opacities.size() - 1.f);
    }

    if (maxValueIndex < int(opacities.size())) {
      range.upper = valueRange.lower
          + maxValueIndex * valueRange.size() / (opacities.size() - 1.f);
    }

    valueRanges.push_back(range);
  }

  return valueRanges;
}

void LinearTransferFunction::precomputeMaxOpacityRanges()
{
  const DataT<float> &opacities = *opacityValues;
  const int maxOpacityDim = opacities.size() - 1;
  const int maxPrecomputedDim = PRECOMPUTED_OPACITY_SUBRANGE_COUNT - 1;

  // compute the diagonal
  for (int i = 0; i < PRECOMPUTED_OPACITY_SUBRANGE_COUNT; i++) {
    // figure out the range of array indices we are going to compare; this is a
    // conservative range of feasible indices that may be used to lookup
    // opacities for any data value within the value range corresponding to [i,
    // i].
    const int checkRangeLow =
        floor(maxOpacityDim * (float)i / maxPrecomputedDim);
    const int checkRangeHigh =
        ceil(maxOpacityDim * (float)i / maxPrecomputedDim);

    float maxOpacity = opacities[checkRangeLow];
    for (int opacityIDX = checkRangeLow; opacityIDX <= checkRangeHigh;
         opacityIDX++)
      maxOpacity = std::max(maxOpacity, opacities[opacityIDX]);

    getSh()->maxOpacityInRange[i][i] = maxOpacity;
  }

  // fill out each column from the diagonal up
  for (int i = 0; i < PRECOMPUTED_OPACITY_SUBRANGE_COUNT; i++) {
    for (int j = i + 1; j < PRECOMPUTED_OPACITY_SUBRANGE_COUNT; j++) {
      // figure out the range of array indices we are going to compare; this is
      // a conservative range of feasible indices that may be used to lookup
      // opacities for any data value within the value range corresponding to
      // [i, j].
      const int checkRangeLow =
          floor(maxOpacityDim * (float)i / maxPrecomputedDim);
      const int checkRangeHigh =
          ceil(maxOpacityDim * (float)j / maxPrecomputedDim);

      float maxOpacity = getSh()->maxOpacityInRange[i][i];
      for (int opacityIDX = checkRangeLow; opacityIDX <= checkRangeHigh;
           opacityIDX++)
        maxOpacity = std::max(maxOpacity, opacities[opacityIDX]);

      getSh()->maxOpacityInRange[i][j] = maxOpacity;
    }
  }
}

} // namespace ospray
