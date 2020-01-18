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

#pragma once

#include "ManagedObject.h"

namespace ospray {
  namespace cpp {

    class ImageOperation
        : public ManagedObject<OSPImageOperation, OSP_IMAGE_OPERATION>
    {
     public:
      ImageOperation(const std::string &type);
      ImageOperation(const ImageOperation &copy);
      ImageOperation(OSPImageOperation existing = nullptr);
    };

    static_assert(sizeof(ImageOperation) == sizeof(OSPImageOperation),
                  "cpp::ImageOperation can't have data members!");

    // Inlined function definitions ///////////////////////////////////////////

    inline ImageOperation::ImageOperation(const std::string &type)
    {
      ospObject = ospNewImageOperation(type.c_str());
    }

    inline ImageOperation::ImageOperation(const ImageOperation &copy)
        : ManagedObject<OSPImageOperation, OSP_IMAGE_OPERATION>(copy.handle())
    {
      ospRetain(copy.handle());
    }

    inline ImageOperation::ImageOperation(OSPImageOperation existing)
        : ManagedObject<OSPImageOperation, OSP_IMAGE_OPERATION>(existing)
    {
    }

  }  // namespace cpp

  OSPTYPEFOR_SPECIALIZATION(cpp::ImageOperation, OSP_IMAGE_OPERATION);

}  // namespace ospray
