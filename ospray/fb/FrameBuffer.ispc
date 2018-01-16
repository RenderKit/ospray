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

#include "fb/FrameBuffer.ih"

void FrameBuffer_Constructor(FrameBuffer *uniform self,
                             void *uniform cClassPtr)
{
  self->cClassPtr  = cClassPtr;
  self->size.x     = 0;
  self->size.y     = 0;
  self->rcpSize.x  = 0.f;
  self->rcpSize.y  = 0.f;
  self->colorBufferFormat = ColorBufferFormat_NONE;
}

void FrameBuffer_set(FrameBuffer *uniform self,
                     const uniform uint32 size_x,
                     const uniform uint32 size_y,
                     uniform int32 colorBufferFormat)
{
  self->size.x     = size_x;
  self->size.y     = size_y;
  self->rcpSize.x  = 1.f/size_x;
  self->rcpSize.y  = 1.f/size_y;
  self->colorBufferFormat = (uniform FrameBuffer_ColorBufferFormat)colorBufferFormat;
}

export void FrameBuffer_set_frameID(void *uniform _self, uniform int32 frameID)
{
  uniform FrameBuffer *uniform self = (uniform FrameBuffer *uniform)_self;
  self->frameID = frameID;
}
