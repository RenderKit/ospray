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

/*! This header is shared with ISPC. */
#pragma once

/*! An enum type that represensts the different data types represented in ospray */
typedef enum
# if __cplusplus >= 201103L
: uint32_t
#endif
{
  //! Object reference type.
  OSP_DEVICE = 100,

  //! Void pointer type.
  OSP_VOID_PTR = 200,

  //! Object reference type.
  OSP_OBJECT = 1000,

  //! Object reference subtypes.
  OSP_CAMERA = 1100,
  OSP_DATA,
  OSP_FRAMEBUFFER,
  OSP_GEOMETRY,
  OSP_LIGHT,
  OSP_MATERIAL,
  OSP_MODEL,
  OSP_RENDERER,
  OSP_TEXTURE,
  OSP_TRANSFER_FUNCTION,
  OSP_VOLUME,
  OSP_PIXEL_OP,

  //! Pointer to a C-style NULL-terminated character string.
  OSP_STRING = 1500,

  //! Character scalar type.
  OSP_CHAR = 2000,

  //! Unsigned character scalar and vector types.
  OSP_UCHAR = 2500, OSP_UCHAR2, OSP_UCHAR3, OSP_UCHAR4,

  //! Signed 16-bit integer scalar.
  OSP_SHORT = 3000,

  //! Unsigned 16-bit integer scalar.
  OSP_USHORT = 3500,

  //! Signed 32-bit integer scalar and vector types.
  OSP_INT = 4000, OSP_INT2, OSP_INT3, OSP_INT4,

  //! Unsigned 32-bit integer scalar and vector types.
  OSP_UINT = 4500, OSP_UINT2, OSP_UINT3, OSP_UINT4,

  //! Signed 64-bit integer scalar and vector types.
  OSP_LONG = 5000, OSP_LONG2, OSP_LONG3, OSP_LONG4,

  //! Unsigned 64-bit integer scalar and vector types.
  OSP_ULONG = 5550, OSP_ULONG2, OSP_ULONG3, OSP_ULONG4,

  //! Single precision floating point scalar and vector types.
  OSP_FLOAT = 6000, OSP_FLOAT2, OSP_FLOAT3, OSP_FLOAT4, OSP_FLOAT3A,

  //! Double precision floating point scalar type.
  OSP_DOUBLE = 7000,

  //! Guard value.
  OSP_UNKNOWN = 22222,


  OSP_RAW = 2500 //XXX OSP_UCHAR, ISPC issue #1246

} OSPDataType;
