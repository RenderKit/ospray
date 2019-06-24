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

// This header is shared with ISPC

#pragma once

// An enum type that represensts the different data types represented in OSPRay
typedef enum
# if __cplusplus >= 201103L
: uint32_t
#endif
{
  // Object reference type.
  OSP_DEVICE = 100,

  // Void pointer type.
  OSP_VOID_PTR = 200,

  // Object reference type.
  OSP_OBJECT = 1000,

  // Object reference subtypes.
  OSP_CAMERA = 1100,
  OSP_DATA,
  OSP_FRAMEBUFFER,
  OSP_GEOMETRY,
  OSP_GEOMETRIC_MODEL,
  OSP_LIGHT,
  OSP_MATERIAL,
  OSP_RENDERER,
  OSP_TEXTURE,
  OSP_TRANSFER_FUNCTION,
  OSP_VOLUME,
  OSP_VOLUMETRIC_MODEL,
  OSP_PIXEL_OP,
  OSP_INSTANCE,
  OSP_WORLD,

  // Pointer to a C-style NULL-terminated character string.
  OSP_STRING = 1500,

  // Character scalar type.
  OSP_CHAR = 2000,

  // Unsigned character scalar and vector types.
  OSP_UCHAR = 2500, OSP_VEC2UC, OSP_VEC3UC, OSP_VEC4UC,

  // Signed 16-bit integer scalar.
  OSP_SHORT = 3000,

  // Unsigned 16-bit integer scalar.
  OSP_USHORT = 3500,

  // Signed 32-bit integer scalar and vector types.
  OSP_INT = 4000, OSP_VEC2I, OSP_VEC3I, OSP_VEC4I,

  // Unsigned 32-bit integer scalar and vector types.
  OSP_UINT = 4500, OSP_VEC2UI, OSP_VEC3UI, OSP_VEC4UI,

  // Signed 64-bit integer scalar and vector types.
  OSP_LONG = 5000, OSP_VEC2L, OSP_VEC3L, OSP_VEC4L,

  // Unsigned 64-bit integer scalar and vector types.
  OSP_ULONG = 5550, OSP_VEC2UL, OSP_VEC3UL, OSP_VEC4UL,

  // Single precision floating point scalar and vector types.
  OSP_FLOAT = 6000, OSP_VEC2F, OSP_VEC3F, OSP_VEC4F, OSP_VEC3FA,

  // Double precision floating point scalar type.
  OSP_DOUBLE = 7000,

  // Signed 32-bit integer N-dimensional box types
  OSP_BOX1I = 8000, OSP_BOX2I, OSP_BOX3I, OSP_BOX4I,

  // Single precision floating point N-dimensional box types
  OSP_BOX1F = 10000, OSP_BOX2F, OSP_BOX3F, OSP_BOX4F,

  // Transformation types
  OSP_LINEAR2F = 12000, OSP_LINEAR3F, OSP_AFFINE2F, OSP_AFFINE3F,

  // Guard value.
  OSP_UNKNOWN = 99999,

  OSP_BYTE = 2500, //XXX OSP_UCHAR, ISPC issue #1246
  OSP_RAW = 2500   //XXX OSP_UCHAR, ISPC issue #1246

} OSPDataType;
