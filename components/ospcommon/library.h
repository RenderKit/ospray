// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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

#include "common.h"
// std
#include <map>
#include <string>

namespace ospcommon {

  class Library
  {
    public:
      /* opens a shared library */
      Library(const std::string& name);

      /* returns address of a symbol from the library */
      void* getSymbol(const std::string& sym) const;

    private:
      Library(void* const lib);
      void *lib;
      friend class LibraryRepository;
  };

  class LibraryRepository
  {
    public:
      static LibraryRepository* getInstance();

      /* add a library to the repo */
      void add(const std::string& name);

      /* returns address of a symbol from any library in the repo */
      void* getSymbol(const std::string& sym) const;

    private:
      static LibraryRepository* instance;
      LibraryRepository();
      std::map<std::string, Library*> repo;
  };
}

