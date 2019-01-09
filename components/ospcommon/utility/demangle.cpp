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

#include "demangle.h"

#ifdef __GNUG__
#  include <cstdlib>
#  include <memory>
#  include <cxxabi.h>
#endif

namespace ospcommon {
  namespace utility {

#ifdef __GNUG__
    std::string demangle(const char* name)
    {
      int status = 0;

      std::unique_ptr<char, void(*)(void*)> res {
          abi::__cxa_demangle(name, NULL, NULL, &status),
          std::free
      };

      return (status == 0) ? res.get() : name ;
    }
#else
    std::string demangle(const char* name)
    {
      return name;
    }
#endif

  } // ::ospcommon::utility
} // ::ospcommon