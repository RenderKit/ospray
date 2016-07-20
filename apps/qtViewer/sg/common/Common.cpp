// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

namespace ospray {
  namespace sg {

    //! parse vec3i from std::string (typically an xml-node's content string) 
    vec3i parseVec3i(const std::string &text) 
    { 
      vec3i ret; 
      int rc = sscanf(text.c_str(),"%i %i %i",&ret.x,&ret.y,&ret.z); 
      assert(rc == 3); 
      return ret; 
    }
    
    //! parse vec2i from std::string (typically an xml-node's content string) 
    vec2i parseVec2i(const std::string &text) 
    { 
      vec2i ret; 
      int rc = sscanf(text.c_str(),"%i %i",&ret.x,&ret.y); 
      assert(rc == 2); 
      return ret; 
    }

    const unsigned char * mapFile(const std::string &fileName)
    {
      FILE *file = fopen(fileName.c_str(), "rb");
      if (!file)
        THROW_SG_ERROR("could not open binary file");
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
