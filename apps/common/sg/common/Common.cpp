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

    /*! return the ospray data type for a given string-ified type */
    OSPDataType getOSPDataTypeFor(const char *string)
    {
      if (string == NULL)                return(OSP_UNKNOWN);
      if (strcmp(string, "char"  ) == 0) return(OSP_CHAR);
      if (strcmp(string, "double") == 0) return(OSP_DOUBLE);
      if (strcmp(string, "float" ) == 0) return(OSP_FLOAT);
      if (strcmp(string, "float2") == 0) return(OSP_FLOAT2);
      if (strcmp(string, "float3") == 0) return(OSP_FLOAT3);
      if (strcmp(string, "float4") == 0) return(OSP_FLOAT4);
      if (strcmp(string, "int"   ) == 0) return(OSP_INT);
      if (strcmp(string, "int2"  ) == 0) return(OSP_INT2);
      if (strcmp(string, "int3"  ) == 0) return(OSP_INT3);
      if (strcmp(string, "int4"  ) == 0) return(OSP_INT4);
      if (strcmp(string, "uchar" ) == 0) return(OSP_UCHAR);
      if (strcmp(string, "uchar2") == 0) return(OSP_UCHAR2);
      if (strcmp(string, "uchar3") == 0) return(OSP_UCHAR3);
      if (strcmp(string, "uchar4") == 0) return(OSP_UCHAR4);
      if (strcmp(string, "short" ) == 0) return(OSP_SHORT);
      if (strcmp(string, "ushort") == 0) return(OSP_USHORT);
      if (strcmp(string, "uint"  ) == 0) return(OSP_UINT);
      if (strcmp(string, "uint2" ) == 0) return(OSP_UINT2);
      if (strcmp(string, "uint3" ) == 0) return(OSP_UINT3);
      if (strcmp(string, "uint4" ) == 0) return(OSP_UINT4);
      return(OSP_UNKNOWN);
    }

    /*! return the ospray data type for a given string-ified type */
    OSPDataType getOSPDataTypeFor(const std::string &typeName)
    { return getOSPDataTypeFor(typeName.c_str()); }


    /*! return the size (in byte) for a given ospray data type */
    size_t sizeOf(const OSPDataType type)
    {
      switch (type) {
      case OSP_VOID_PTR:
      case OSP_OBJECT:
      case OSP_CAMERA:
      case OSP_DATA:
      case OSP_DEVICE:
      case OSP_FRAMEBUFFER:
      case OSP_GEOMETRY:
      case OSP_LIGHT:
      case OSP_MATERIAL:
      case OSP_MODEL:
      case OSP_RENDERER:
      case OSP_TEXTURE:
      case OSP_TRANSFER_FUNCTION:
      case OSP_VOLUME:
      case OSP_PIXEL_OP:
      case OSP_STRING:    return sizeof(void *);
      case OSP_CHAR:      return sizeof(int8_t);
      case OSP_UCHAR:     return sizeof(uint8_t);
      case OSP_UCHAR2:    return sizeof(vec2uc);
      case OSP_UCHAR3:    return sizeof(vec3uc);
      case OSP_UCHAR4:    return sizeof(vec4uc);
      case OSP_SHORT:     return sizeof(int16_t);
      case OSP_USHORT:    return sizeof(uint16_t);
      case OSP_INT:       return sizeof(int32_t);
      case OSP_INT2:      return sizeof(vec2i);
      case OSP_INT3:      return sizeof(vec3i);
      case OSP_INT4:      return sizeof(vec4i);
      case OSP_UINT:      return sizeof(uint32_t);
      case OSP_UINT2:     return sizeof(vec2ui);
      case OSP_UINT3:     return sizeof(vec3ui);
      case OSP_UINT4:     return sizeof(vec4ui);
      case OSP_LONG:      return sizeof(int64_t);
      case OSP_LONG2:     return sizeof(vec2l);
      case OSP_LONG3:     return sizeof(vec3l);
      case OSP_LONG4:     return sizeof(vec4l);
      case OSP_ULONG:     return sizeof(uint64_t);
      case OSP_ULONG2:    return sizeof(vec2ul);
      case OSP_ULONG3:    return sizeof(vec3ul);
      case OSP_ULONG4:    return sizeof(vec4ul);
      case OSP_FLOAT:     return sizeof(float);
      case OSP_FLOAT2:    return sizeof(vec2f);
      case OSP_FLOAT3:    return sizeof(vec3f);
      case OSP_FLOAT4:    return sizeof(vec4f);
      case OSP_FLOAT3A:   return sizeof(vec3fa);
      case OSP_DOUBLE:    return sizeof(double);
      case OSP_UNKNOWN:   break;
      };

      std::stringstream error;
      error << __FILE__ << ":" << __LINE__ << ": unknown OSPDataType "
            << (int)type;
      throw std::runtime_error(error.str());
    }

  } // ::ospray::sg
} // ::ospray
