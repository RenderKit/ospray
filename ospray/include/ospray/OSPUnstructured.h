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

// OSPRay cell types definition for unstructured volumes, values are set to match VTK
typedef enum
# if __cplusplus >= 201103L
: uint8_t
#endif
{
  OSP_TETRAHEDRON = 10,
  OSP_HEXAHEDRON = 12,
  OSP_WEDGE = 13,
  OSP_PYRAMID = 14
} OSPUnstructuredCellType;
