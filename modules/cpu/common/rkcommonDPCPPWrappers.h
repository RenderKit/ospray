// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef OSPRAY_TARGET_DPCPP
#include <CL/sycl.hpp>
#include <type_traits>

#include "common/OSPCommon.ih"
#include "rkcommon/math/AffineSpace.h"
#include "rkcommon/math/LinearSpace.h"
#include "rkcommon/math/math.h"
#include "rkcommon/math/rkmath.h"
#include "rkcommon/math/vec.h"
#include "rkcommon/utility/random.h"

using namespace rkcommon::utility;
using namespace rkcommon::math;

#endif
