// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
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

// ospray
#include "ospray/common/Library.h"
#include "ospray/volume/Volume.h"
// stl
#include <map>

namespace ospray {

  Volume *Volume::createInstance(std::string type) 
  {
    //! Function pointer type for creating a concrete instance of a subtype of this class.
    typedef Volume *(*creationFunctionPointer)();

    //! Function pointers corresponding to each subtype.
    static std::map<std::string, creationFunctionPointer> symbolRegistry;

    //! Find the creation function for the subtype if not already known.
    if (symbolRegistry.count(type) == 0) {

      //! Construct the name of the creation function to look for.
      std::string creationFunctionName = "ospray_create_volume_" + type;

      //! Look for the named function.
      symbolRegistry[type] = (creationFunctionPointer) getSymbol(creationFunctionName);

      //! The named function may not be found if the requested subtype is not known.
      if (!symbolRegistry[type] && ospray::logLevel >= 1) 
        std::cerr << "  ospray::Volume  WARNING: unrecognized subtype '" + type + "'." << std::endl;
    }

    //! Create a concrete instance of the requested subtype.
    Volume *volume = (symbolRegistry[type]) ? (*symbolRegistry[type])() : NULL;

    //! Denote the subclass type in the ManagedObject base class.
    if (volume) volume->managedObjectType = OSP_VOLUME;

    return(volume);

  }

} // ::ospray

