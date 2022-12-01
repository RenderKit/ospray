// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "OSPConfig.h"

#if EMBREE_VERSION_MAJOR == 3

#define RTC_SYCL_INDIRECTLY_CALLABLE
#ifdef ISPC
#include "embree3/rtcore.isph"
#else
#include "embree3/rtcore.h"
#endif

#else

#ifdef OSPRAY_TARGET_SYCL
#include <sycl/sycl.hpp>
#endif

#ifdef ISPC
#include "embree4/rtcore.isph"
#else
#include "embree4/rtcore.h"
#endif

#endif
