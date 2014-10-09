/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "device.h"
#include "ospray/common/OspCommon.h"
#include "embree2/rtcore.h"

namespace ospray {
  void error_handler(const RTCError code, const char *str);
  namespace api {
    Device *Device::current = NULL;

    Device::Device() {
      rtcSetErrorFunction(error_handler);
    }
  }
}
