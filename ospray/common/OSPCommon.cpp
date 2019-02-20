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

#include "OSPCommon.h"
#include "api/Device.h"

#include <map>

namespace ospray {

  /*! 64-bit malloc. allows for alloc'ing memory larger than 4GB */
  extern "C" void *malloc64(size_t size)
  {
    return alignedMalloc(size);
  }

  /*! 64-bit malloc. allows for alloc'ing memory larger than 4GB */
  extern "C" void free64(void *ptr)
  {
    return alignedFree(ptr);
  }

  WarnOnce::WarnOnce(const std::string &s, uint32_t postAtLogLevel)
    : s(s)
  {
    postStatusMsg(postAtLogLevel) << "Warning: " << s
                                  << " (only reporting first occurrence)";
  }

  void initFromCommandLine(int *_ac, const char ***_av)
  {
    auto &device = ospray::api::Device::current;

    if (_ac && _av) {
      int &ac = *_ac;
      auto &av = *_av;
      for (int i=1;i<ac;) {
        std::string parm = av[i];
        if (parm == "--osp:debug") {
          device->setParam("debug", true);
          device->setParam("logOutput", std::string("cout"));
          device->setParam("errorOutput", std::string("cerr"));
          removeArgs(ac,av,i,1);
        } else if (parm == "--osp:verbose") {
          device->setParam("logLevel", 1);
          device->setParam("logOutput", std::string("cout"));
          device->setParam("errorOutput", std::string("cerr"));
          removeArgs(ac,av,i,1);
        } else if (parm == "--osp:vv") {
          device->setParam("logLevel", 2);
          device->setParam("logOutput", std::string("cout"));
          device->setParam("errorOutput", std::string("cerr"));
          removeArgs(ac,av,i,1);
        } else if (parm == "--osp:loglevel") {
          if (i+1<ac) {
            device->setParam("logLevel", atoi(av[i+1]));
            removeArgs(ac,av,i,2);
          } else {
            postStatusMsg("<n> argument required for --osp:loglevel!");
            removeArgs(ac,av,i,1);
          }
        } else if (parm == "--osp:logoutput") {
          if (i+1<ac) {
            std::string dst = av[i+1];

            if (dst == "cout" || dst == "cerr")
              device->setParam("logOutput", dst);
            else
              postStatusMsg("You must use 'cout' or 'cerr' for --osp:logoutput!");

            removeArgs(ac,av,i,2);
          } else {
            postStatusMsg("<dst> argument required for --osp:logoutput!");
            removeArgs(ac,av,i,1);
          }
        } else if (parm == "--osp:erroroutput") {
          if (i+1<ac) {
            std::string dst = av[i+1];

            if (dst == "cout" || dst == "cerr")
              device->setParam("errorOutput", dst);
            else {
              postStatusMsg("You must use 'cout' or 'cerr' for"
                            " --osp:erroroutput!");
            }

            removeArgs(ac,av,i,2);
          } else {
            postStatusMsg("<dst> argument required for --osp:erroroutput!");
            removeArgs(ac,av,i,1);
          }
        } else if (parm == "--osp:numthreads" || parm == "--osp:num-threads") {
          if (i+1<ac) {
            device->setParam("numThreads", atoi(av[i+1]));
            removeArgs(ac,av,i,2);
          } else {
            postStatusMsg("<n> argument required for --osp:numthreads");
            removeArgs(ac,av,i,1);
          }
        } else if (parm == "--osp:setaffinity" || parm == "--osp:affinity") {
          if (i+1<ac) {
            device->setParam<bool>("setAffinity", atoi(av[i+1]));
            removeArgs(ac,av,i,2);
          } else {
            postStatusMsg("<n> argument required for --osp:setaffinity!");
            removeArgs(ac,av,i,1);
          }
        } else {
          ++i;
        }
      }
    }
  }

  size_t sizeOf(const OSPDataType type) {
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
    case OSP_CHAR:      return sizeof(int8);
    case OSP_UCHAR:     return sizeof(uint8);
    case OSP_UCHAR2:    return sizeof(vec2uc);
    case OSP_UCHAR3:    return sizeof(vec3uc);
    case OSP_UCHAR4:    return sizeof(vec4uc);
    case OSP_SHORT:     return sizeof(int16);
    case OSP_USHORT:    return sizeof(uint16);
    case OSP_INT:       return sizeof(int32);
    case OSP_INT2:      return sizeof(vec2i);
    case OSP_INT3:      return sizeof(vec3i);
    case OSP_INT4:      return sizeof(vec4i);
    case OSP_UINT:      return sizeof(uint32);
    case OSP_UINT2:     return sizeof(vec2ui);
    case OSP_UINT3:     return sizeof(vec3ui);
    case OSP_UINT4:     return sizeof(vec4ui);
    case OSP_LONG:      return sizeof(int64);
    case OSP_LONG2:     return sizeof(vec2l);
    case OSP_LONG3:     return sizeof(vec3l);
    case OSP_LONG4:     return sizeof(vec4l);
    case OSP_ULONG:     return sizeof(uint64);
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

  OSPDataType typeForString(const char *string)
  {
    if (string == nullptr)             return(OSP_UNKNOWN);
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

  std::string stringForType(OSPDataType type)
  {
    switch (type) {
    case OSP_VOID_PTR:          return "void_ptr";
    case OSP_OBJECT:            return "object";
    case OSP_CAMERA:            return "camera";
    case OSP_DATA:              return "data";
    case OSP_DEVICE:            return "device";
    case OSP_FRAMEBUFFER:       return "framebuffer";
    case OSP_GEOMETRY:          return "geometry";
    case OSP_LIGHT:             return "light";
    case OSP_MATERIAL:          return "material";
    case OSP_MODEL:             return "model";
    case OSP_RENDERER:          return "renderer";
    case OSP_TEXTURE:           return "texture";
    case OSP_TRANSFER_FUNCTION: return "transfer_function";
    case OSP_VOLUME:            return "volume";
    case OSP_PIXEL_OP:          return "pixel_op";
    case OSP_STRING:            return "string";
    case OSP_CHAR:              return "char";
    case OSP_UCHAR:             return "uchar";
    case OSP_UCHAR2:            return "uchar2";
    case OSP_UCHAR3:            return "uchar3";
    case OSP_UCHAR4:            return "uchar4";
    case OSP_SHORT:             return "short";
    case OSP_USHORT:            return "ushort";
    case OSP_INT:               return "int";
    case OSP_INT2:              return "int2";
    case OSP_INT3:              return "int3";
    case OSP_INT4:              return "int4";
    case OSP_UINT:              return "uint";
    case OSP_UINT2:             return "uint2";
    case OSP_UINT3:             return "uint3";
    case OSP_UINT4:             return "uint4";
    case OSP_LONG:              return "long";
    case OSP_LONG2:             return "long2";
    case OSP_LONG3:             return "long3";
    case OSP_LONG4:             return "long4";
    case OSP_ULONG:             return "ulong";
    case OSP_ULONG2:            return "ulong2";
    case OSP_ULONG3:            return "ulong3";
    case OSP_ULONG4:            return "ulong4";
    case OSP_FLOAT:             return "float";
    case OSP_FLOAT2:            return "float2";
    case OSP_FLOAT3:            return "float3";
    case OSP_FLOAT4:            return "float4";
    case OSP_FLOAT3A:           return "float3a";
    case OSP_DOUBLE:            return "double";
    case OSP_UNKNOWN:           break;
    };

    std::stringstream error;
    error << __FILE__ << ":" << __LINE__ << ": unknown OSPDataType "
          << (int)type;
    throw std::runtime_error(error.str());
  }

  size_t sizeOf(const OSPTextureFormat type)
  {
    switch (type) {
      case OSP_TEXTURE_RGBA8:
      case OSP_TEXTURE_SRGBA:          return sizeof(uint32);
      case OSP_TEXTURE_RGBA32F:        return sizeof(vec4f);
      case OSP_TEXTURE_RGB8:
      case OSP_TEXTURE_SRGB:           return sizeof(vec3uc);
      case OSP_TEXTURE_RGB32F:         return sizeof(vec3f);
      case OSP_TEXTURE_R8:
      case OSP_TEXTURE_L8:             return sizeof(uint8);
      case OSP_TEXTURE_RA8:
      case OSP_TEXTURE_LA8:            return sizeof(vec2uc);
      case OSP_TEXTURE_R32F:           return sizeof(float);
      case OSP_TEXTURE_FORMAT_INVALID: break;
    }

    std::stringstream error;
    error << __FILE__ << ":" << __LINE__ << ": unknown OSPTextureFormat "
          << (int)type;
    throw std::runtime_error(error.str());
  }

  uint32_t logLevel()
  {
    return ospray::api::Device::current->logLevel;
  }

  OSPError loadLocalModule(const std::string &name)
  {
    std::string libName = "ospray_module_" + name;
    loadLibrary(libName);

    std::string initSymName = "ospray_init_module_" + name;
    void*initSym = getSymbol(initSymName);
    if (!initSym) {
      throw std::runtime_error("#osp:api: could not find module initializer "
                               +initSymName);
    }

    void (*initMethod)() = (void(*)())initSym;

    if (!initMethod)
      return OSP_INVALID_ARGUMENT;

    try {
      initMethod();
    } catch (...) {
      return OSP_UNKNOWN_ERROR;
    }

    return OSP_NO_ERROR;
  }

  StatusMsgStream postStatusMsg(uint32_t postAtLogLevel)
  {
    return StatusMsgStream(postAtLogLevel);
  }

  void postStatusMsg(const std::stringstream &msg, uint32_t postAtLogLevel)
  {
    postStatusMsg(msg.str(), postAtLogLevel);
  }

  void postStatusMsg(const std::string &msg, uint32_t postAtLogLevel)
  {
    if (logLevel() >= postAtLogLevel && api::deviceIsSet())
      ospray::api::Device::current->msg_fcn((msg + '\n').c_str());
  }

  void handleError(OSPError e, const std::string &message)
  {
    if (api::deviceIsSet()) {
      auto &device = api::currentDevice();

      device.lastErrorCode = e;
      device.lastErrorMsg  = message;

      device.error_fcn(e, message.c_str());
    } else {
      // NOTE: No device, but something should still get printed for the user to
      //       debug the calling application.
      std::cerr << "#osp: INITIALIZATION ERROR --> " << message << std::endl;
    }
  }

  void postTraceMsg(const std::string &message)
  {
    if (api::deviceIsSet()) {
      auto &device = api::currentDevice();
      device.trace_fcn(message.c_str());
    }
  }

  size_t translatedHash(size_t v)
  {
    static std::map<size_t, size_t> id_translation;

    auto itr = id_translation.find(v);
    if (itr == id_translation.end()) {
      static size_t newIndex = 0;
      id_translation[v] = newIndex;
      return newIndex++;
    } else {
      return id_translation[v];
    }
  }

} // ::ospray

