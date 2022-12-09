// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef OSPRAY_TARGET_SYCL
#include <sycl/sycl.hpp>
#endif

#ifdef ISPC
#include "embree4/rtcore.isph"
#else
#include "embree4/rtcore.h"
#endif
