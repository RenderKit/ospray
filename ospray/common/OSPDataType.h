// ======================================================================== //
// Copyright 2009-2014 Intel Corporation                                    //
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
typedef enum {

  //! Void pointer type.
  OSP_VOID_PTR,

  //! Object reference type.
  OSP_OBJECT,

  //! Object reference subtypes.
  OSP_CAMERA,
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

  //! Pointer to a C-style NULL-terminated character string.
  OSP_STRING,

  //! Character scalar type.
  OSP_CHAR,

  //! Unsigned character scalar and vector types.
  OSP_UCHAR, OSP_UCHAR2, OSP_UCHAR3, OSP_UCHAR4,

  //! Signed integer scalar and vector types.
  OSP_INT, OSP_INT2, OSP_INT3, OSP_INT4,

  //! Unsigned integer scalar and vector types.
  OSP_UINT, OSP_UINT2, OSP_UINT3, OSP_UINT4,

  //! Single precision floating point scalar and vector types.
  OSP_FLOAT, OSP_FLOAT2, OSP_FLOAT3, OSP_FLOAT4, OSP_FLOAT3A,

  //! Guard value.
  OSP_UNKNOWN,

} OSPDataType;

