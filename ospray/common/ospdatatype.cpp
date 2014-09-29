/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "ospcommon.h"
#include "ospdatatype.h"

size_t sizeOf(OSPDataType type) {

  switch (type) {
  case OSP_VOID_PTR:  return sizeof(void *);
  case OSP_OBJECT:    return sizeof(void *);
  case OSP_CHAR:      return sizeof(int8);
  case OSP_UCHAR:     return sizeof(uint8);
  case OSP_UCHAR2:    return sizeof(embree::Vec2<uint8>);
  case OSP_UCHAR3:    return sizeof(embree::Vec3<uint8>);
  case OSP_UCHAR4:    return sizeof(uint32);
  case OSP_INT:       return sizeof(int32);
  case OSP_INT2:      return sizeof(embree::Vec2<int32>);
  case OSP_INT3:      return sizeof(embree::Vec3<int32>);
  case OSP_INT4:      return sizeof(embree::Vec4<int32>);
  case OSP_UINT:      return sizeof(uint32);
  case OSP_UINT2:     return sizeof(embree::Vec2<uint32>);
  case OSP_UINT3:     return sizeof(embree::Vec3<uint32>);
  case OSP_UINT4:     return sizeof(embree::Vec4<uint32>);
  case OSP_FLOAT:     return sizeof(float);
  case OSP_FLOAT2:    return sizeof(embree::Vec2<float>);
  case OSP_FLOAT3:    return sizeof(embree::Vec3<float>);
  case OSP_FLOAT4:    return sizeof(embree::Vec4<float>);
  case OSP_FLOAT3A:   return sizeof(embree::Vec3fa);
  default: break;
  };

  std::stringstream error;
  error << __FILE__ << ":" << __LINE__ << ": unknown OSPDataType " << (int)type;
  throw std::runtime_error(error.str());
}

