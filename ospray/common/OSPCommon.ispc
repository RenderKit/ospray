// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "OSPCommon.ih"

void error_handler(const RTCError code, const int8 *str)
{
  print("Embree: ");
  switch (code) {
  case RTC_ERROR_UNKNOWN:
    print("RTC_UNKNOWN_ERROR");
    break;
  case RTC_ERROR_INVALID_ARGUMENT:
    print("RTC_INVALID_ARGUMENT");
    break;
  case RTC_ERROR_INVALID_OPERATION:
    print("RTC_INVALID_OPERATION");
    break;
  case RTC_ERROR_OUT_OF_MEMORY:
    print("RTC_OUT_OF_MEMORY");
    break;
  case RTC_ERROR_UNSUPPORTED_CPU:
    print("RTC_UNSUPPORTED_CPU");
    break;
  default:
    print("invalid error code");
    break;
  }
  if (str) {
    print("(%)", str);
  }
  assert(0);
}

export void delete_uniform(void *uniform uptr)
{
  delete uptr;
}
