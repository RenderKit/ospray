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

#include "OSPCommon.ih"

void error_handler(const RTCError code, const int8* str)
{
  print("Embree: ");
  switch (code) {
  case RTC_ERROR_UNKNOWN           : print("RTC_UNKNOWN_ERROR");     break;
  case RTC_ERROR_INVALID_ARGUMENT  : print("RTC_INVALID_ARGUMENT");  break;
  case RTC_ERROR_INVALID_OPERATION : print("RTC_INVALID_OPERATION"); break;
  case RTC_ERROR_OUT_OF_MEMORY     : print("RTC_OUT_OF_MEMORY");     break;
  case RTC_ERROR_UNSUPPORTED_CPU   : print("RTC_UNSUPPORTED_CPU");   break;
  default                          : print("invalid error code");    break;
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
