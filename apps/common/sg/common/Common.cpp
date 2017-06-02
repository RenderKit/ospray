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

// scene graph common stuff
#include "Common.h"

// stdlib, for mmap
#include <sys/types.h>
#include <sys/stat.h>
#ifdef _WIN32
#  include <windows.h>
#else
#  include <sys/mman.h>
#endif
#include <fcntl.h>

// O_LARGEFILE is a GNU extension.
#ifdef __APPLE__
#define  O_LARGEFILE  0
#endif

#include <iostream>
#include <sstream>

namespace ospray {
  namespace sg {

    const unsigned char * mapFile(const std::string &fileName)
    {
      FILE *file = fopen(fileName.c_str(), "rb");
      if (!file) {
        std::cout << "========================================================" << std::endl;
        std::cout << "WARNING: The ospray/sg .xml file you were trying to open" << std::endl;
        std::cout << "does ***NOT*** come with an accompanying .xmlbin file." << std::endl;
        std::cout << "Note this _may_ be OK in some cases, but if you do get" << std::endl;
        std::cout << "undefined behavior or core dumps please make sure that" << std::endl;
        std::cout << "you are not missing this file (ie, a common cause is to" << std::endl;
        std::cout << "use a zipped .xmlbin file that we cannot directly use." << std::endl;
        std::cout << "========================================================" << std::endl;
        return NULL;
      }
        // THROW_SG_ERROR("could not open binary file");
      fseek(file, 0, SEEK_END);
      ssize_t fileSize =
#ifdef _WIN32
        _ftelli64(file);
#else
        ftell(file);
#endif
      fclose(file);

#ifdef _WIN32
      HANDLE fileHandle = CreateFile(fileName.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
      if (fileHandle == nullptr)
        THROW_SG_ERROR("could not open file '" + fileName + "' (error " + std::to_string(GetLastError()) + ")\n");
      HANDLE fileMappingHandle = CreateFileMapping(fileHandle, nullptr, PAGE_READONLY, 0, 0, nullptr);
      if (fileMappingHandle == nullptr)
        THROW_SG_ERROR("could not create file mapping (error " + std::to_string(GetLastError()) + ")\n");
#else
      int fd = ::open(fileName.c_str(), O_LARGEFILE | O_RDONLY);
      if (fd == -1)
        THROW_SG_ERROR("could not open file '" + fileName + "'\n");
#endif

      return (unsigned char *)
#ifdef _WIN32
        MapViewOfFile(fileMappingHandle, FILE_MAP_READ, 0, 0, fileSize);
#else
        mmap(nullptr, fileSize, PROT_READ, MAP_SHARED, fd, 0);
#endif
    }

  } // ::ospray::sg
} // ::ospray
