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

/*! OSPRay format constants for Texture creation */
typedef enum
# if __cplusplus >= 201103L
: uint32_t
#endif
{
  OSP_TEXTURE_RGBA8,
  OSP_TEXTURE_SRGBA,
  OSP_TEXTURE_RGBA32F,
  OSP_TEXTURE_RGB8,
  OSP_TEXTURE_SRGB,
  OSP_TEXTURE_RGB32F,
  OSP_TEXTURE_R8,
  OSP_TEXTURE_R32F,
  OSP_TEXTURE_L8,
  OSP_TEXTURE_RA8,
  OSP_TEXTURE_LA8,
  /*! denotes an unknown texture format, so we can properly initialize parameters */
  OSP_TEXTURE_FORMAT_INVALID = 0xff,
/* TODO
  OSP_LogLuv,
  OSP_RGBA16F
  OSP_RGB16F
  OSP_RGBE, // radiance hdr
  compressed (RGTC, BPTC, ETC, ...)
*/
} OSPTextureFormat;

/*! flags that can be passed to ospNewTexture2D(); can be OR'ed together */
typedef enum
# if __cplusplus >= 201103L
: uint32_t
#endif
{
  OSP_TEXTURE_SHARED_BUFFER = (1<<0),
  OSP_TEXTURE_FILTER_NEAREST = (1<<1) /*!< use nearest-neighbor interpolation rather than the default bilinear interpolation */
} OSPTextureCreationFlags;

