// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "LinearTransferFunction.h"
#include "LinearTransferFunction_ispc.h"

namespace ospray {

LinearTransferFunction::LinearTransferFunction()
{
  ispcEquivalent = ispc::LinearTransferFunction_create();
}

void LinearTransferFunction::commit()
{
  TransferFunction::commit();

  colorValues = getParamDataT<vec3f>("color", true);
  opacityValues = getParamDataT<float>("opacity", true);

  ispc::LinearTransferFunction_set(
      ispcEquivalent, ispc(colorValues), ispc(opacityValues));
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

} // namespace ospray
