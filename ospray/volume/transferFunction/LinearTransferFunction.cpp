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

    colorValues = getParamDataT<vec3f>("color");
    opacityValues = getParamDataT<float>("opacity");

    ispc::LinearTransferFunction_set(
        ispcEquivalent, ispc(colorValues), ispc(opacityValues));
  }

  std::string LinearTransferFunction::toString() const
  {
    return "ospray::LinearTransferFunction";
  }

  std::vector<range1i> LinearTransferFunction::getPositiveOpacityIndexRanges() const
  {
    std::vector<range1i> intervals;

    range1i interval;
    bool rangeActive = false;

    const DataT<float> &opacities = *opacityValues;

    for (int i = 0; i < int(opacities.size()); i++) {
      if (opacities[i] > 0.f && !rangeActive) {
        rangeActive    = true;
        interval.lower = i;
      } else if (opacities[i] <= 0.f && rangeActive) {
        rangeActive    = false;
        interval.upper = i;
        intervals.push_back(interval);
      }
    }

    // special case for final value
    if (opacities[opacities.size()-1] > 0.f) {
      if (rangeActive) {
        rangeActive    = false;
        interval.upper = opacities.size();
        intervals.push_back(interval);
      } else {
        throw std::runtime_error("getPositiveOpacityIndexRanges() error");
      }
    }

    return intervals;
  }

  std::vector<range1f> LinearTransferFunction::getPositiveOpacityValueRanges() const
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

  OSP_REGISTER_TRANSFER_FUNCTION(LinearTransferFunction, piecewise_linear);

} // namespace ospray
