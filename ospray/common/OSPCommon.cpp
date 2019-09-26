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

  size_t sizeOf(OSPDataType type)
  {
    switch (type) {
    case OSP_VOID_PTR:
    case OSP_OBJECT:
    case OSP_CAMERA:
    case OSP_DATA:
    case OSP_DEVICE:
    case OSP_FRAMEBUFFER:
    case OSP_FUTURE:
    case OSP_GEOMETRIC_MODEL:
    case OSP_GEOMETRY:
    case OSP_GROUP:
    case OSP_IMAGE_OPERATION:
    case OSP_INSTANCE:
    case OSP_LIGHT:
    case OSP_MATERIAL:
    case OSP_RENDERER:
    case OSP_TEXTURE:
    case OSP_TRANSFER_FUNCTION:
    case OSP_VOLUME:
    case OSP_VOLUMETRIC_MODEL:
    case OSP_WORLD:
    case OSP_STRING:    return sizeof(void *);
    case OSP_BOOL:      return sizeof(bool);
    case OSP_CHAR:      return sizeof(int8);
    case OSP_UCHAR:     return sizeof(uint8);
    case OSP_VEC2UC:    return sizeof(vec2uc);
    case OSP_VEC3UC:    return sizeof(vec3uc);
    case OSP_VEC4UC:    return sizeof(vec4uc);
    case OSP_SHORT:     return sizeof(int16);
    case OSP_USHORT:    return sizeof(uint16);
    case OSP_INT:       return sizeof(int32);
    case OSP_VEC2I:     return sizeof(vec2i);
    case OSP_VEC3I:     return sizeof(vec3i);
    case OSP_VEC4I:     return sizeof(vec4i);
    case OSP_UINT:      return sizeof(uint32);
    case OSP_VEC2UI:    return sizeof(vec2ui);
    case OSP_VEC3UI:    return sizeof(vec3ui);
    case OSP_VEC4UI:    return sizeof(vec4ui);
    case OSP_LONG:      return sizeof(int64);
    case OSP_VEC2L:     return sizeof(vec2l);
    case OSP_VEC3L:     return sizeof(vec3l);
    case OSP_VEC4L:     return sizeof(vec4l);
    case OSP_ULONG:     return sizeof(uint64);
    case OSP_VEC2UL:    return sizeof(vec2ul);
    case OSP_VEC3UL:    return sizeof(vec3ul);
    case OSP_VEC4UL:    return sizeof(vec4ul);
    case OSP_FLOAT:     return sizeof(float);
    case OSP_VEC2F:     return sizeof(vec2f);
    case OSP_VEC3F:     return sizeof(vec3f);
    case OSP_VEC4F:     return sizeof(vec4f);
    case OSP_DOUBLE:    return sizeof(double);
    case OSP_BOX1I:     return sizeof(box1i);
    case OSP_BOX2I:     return sizeof(box2i);
    case OSP_BOX3I:     return sizeof(box3i);
    case OSP_BOX4I:     return sizeof(box4i);
    case OSP_BOX1F:     return sizeof(box1f);
    case OSP_BOX2F:     return sizeof(box2f);
    case OSP_BOX3F:     return sizeof(box3f);
    case OSP_BOX4F:     return sizeof(box4f);
    case OSP_LINEAR2F:  return sizeof(linear2f);
    case OSP_LINEAR3F:  return sizeof(linear3f);
    case OSP_AFFINE2F:  return sizeof(affine2f);
    case OSP_AFFINE3F:  return sizeof(affine3f);
    case OSP_UNKNOWN:   return 0;
    }

    std::stringstream error;
    error << __FILE__ << ":" << __LINE__ << ": unknown OSPDataType "
          << (int)type;
    throw std::runtime_error(error.str());
  }

  OSPDataType typeOf(const char *string)
  {
    if (string == nullptr)             return(OSP_UNKNOWN);
    if (strcmp(string, "bool"  ) == 0) return(OSP_BOOL);
    if (strcmp(string, "char"  ) == 0) return(OSP_CHAR);
    if (strcmp(string, "double") == 0) return(OSP_DOUBLE);
    if (strcmp(string, "float" ) == 0) return(OSP_FLOAT);
    if (strcmp(string, "vec2f" ) == 0) return(OSP_VEC2F);
    if (strcmp(string, "vec3f" ) == 0) return(OSP_VEC3F);
    if (strcmp(string, "vec4f" ) == 0) return(OSP_VEC4F);
    if (strcmp(string, "int"   ) == 0) return(OSP_INT);
    if (strcmp(string, "vec2i" ) == 0) return(OSP_VEC2I);
    if (strcmp(string, "vec3i" ) == 0) return(OSP_VEC3I);
    if (strcmp(string, "vec4i" ) == 0) return(OSP_VEC4I);
    if (strcmp(string, "uchar" ) == 0) return(OSP_UCHAR);
    if (strcmp(string, "vec2uc") == 0) return(OSP_VEC2UC);
    if (strcmp(string, "vec3uc") == 0) return(OSP_VEC3UC);
    if (strcmp(string, "vec4uc") == 0) return(OSP_VEC4UC);
    if (strcmp(string, "short" ) == 0) return(OSP_SHORT);
    if (strcmp(string, "ushort") == 0) return(OSP_USHORT);
    if (strcmp(string, "uint"  ) == 0) return(OSP_UINT);
    if (strcmp(string, "uint2" ) == 0) return(OSP_VEC2UI);
    if (strcmp(string, "uint3" ) == 0) return(OSP_VEC3UI);
    if (strcmp(string, "uint4" ) == 0) return(OSP_VEC4UI);
    return(OSP_UNKNOWN);
  }

  std::string stringFor(OSPDataType type)
  {
    switch (type) {
    case OSP_VOID_PTR:          return "void_ptr";
    case OSP_OBJECT:            return "object";
    case OSP_CAMERA:            return "camera";
    case OSP_DATA:              return "data";
    case OSP_DEVICE:            return "device";
    case OSP_FRAMEBUFFER:       return "framebuffer";
    case OSP_FUTURE:            return "future";
    case OSP_GEOMETRIC_MODEL:   return "geometric_model";
    case OSP_GEOMETRY:          return "geometry";
    case OSP_GROUP:             return "group";
    case OSP_IMAGE_OPERATION:   return "image_operation";
    case OSP_INSTANCE:          return "instance";
    case OSP_LIGHT:             return "light";
    case OSP_MATERIAL:          return "material";
    case OSP_RENDERER:          return "renderer";
    case OSP_TEXTURE:           return "texture";
    case OSP_TRANSFER_FUNCTION: return "transfer_function";
    case OSP_VOLUME:            return "volume";
    case OSP_VOLUMETRIC_MODEL:  return "volumetric_model";
    case OSP_WORLD:             return "world";
    case OSP_STRING:            return "string";
    case OSP_BOOL:              return "bool";
    case OSP_CHAR:              return "char";
    case OSP_UCHAR:             return "uchar";
    case OSP_VEC2UC:            return "vec2uc";
    case OSP_VEC3UC:            return "vec3uc";
    case OSP_VEC4UC:            return "vec4uc";
    case OSP_SHORT:             return "short";
    case OSP_USHORT:            return "ushort";
    case OSP_INT:               return "int";
    case OSP_VEC2I:             return "vec2i";
    case OSP_VEC3I:             return "vec3i";
    case OSP_VEC4I:             return "vec4i";
    case OSP_UINT:              return "uint";
    case OSP_VEC2UI:            return "vec2ui";
    case OSP_VEC3UI:            return "vec3ui";
    case OSP_VEC4UI:            return "vec4ui";
    case OSP_LONG:              return "long";
    case OSP_VEC2L:             return "vec2l";
    case OSP_VEC3L:             return "vec3l";
    case OSP_VEC4L:             return "vec4l";
    case OSP_ULONG:             return "ulong";
    case OSP_VEC2UL:            return "vec2ul";
    case OSP_VEC3UL:            return "vec3ul";
    case OSP_VEC4UL:            return "vec4ul";
    case OSP_FLOAT:             return "float";
    case OSP_VEC2F:             return "vec2f";
    case OSP_VEC3F:             return "vec3f";
    case OSP_VEC4F:             return "vec4f";
    case OSP_DOUBLE:            return "double";
    case OSP_BOX1I:             return "box1i";
    case OSP_BOX2I:             return "box2i";
    case OSP_BOX3I:             return "box3i";
    case OSP_BOX4I:             return "box4i";
    case OSP_BOX1F:             return "box1f";
    case OSP_BOX2F:             return "box2f";
    case OSP_BOX3F:             return "box3f";
    case OSP_BOX4F:             return "box4f";
    case OSP_LINEAR2F:          return "linear2f";
    case OSP_LINEAR3F:          return "linear3f";
    case OSP_AFFINE2F:          return "affine2f";
    case OSP_AFFINE3F:          return "affine3f";
    case OSP_UNKNOWN:
      return "unknown";
    }

    std::stringstream error;
    error << __FILE__ << ":" << __LINE__ << ": undefined OSPDataType "
          << (int)type;
    throw std::runtime_error(error.str());
  }

  std::string stringFor(OSPTextureFormat format)
  {
    switch (format) {
    case OSP_TEXTURE_RGBA8:
      return "rgba8";
    case OSP_TEXTURE_SRGBA:
      return "srgba";
    case OSP_TEXTURE_RGBA32F:
      return "rgba32f";
    case OSP_TEXTURE_RGB8:
      return "rgb8";
    case OSP_TEXTURE_SRGB:
      return "srgb";
    case OSP_TEXTURE_RGB32F:
      return "rgb32f";
    case OSP_TEXTURE_R8:
      return "r8";
    case OSP_TEXTURE_L8:
      return "l8";
    case OSP_TEXTURE_RA8:
      return "ra8";
    case OSP_TEXTURE_LA8:
      return "la8";
    case OSP_TEXTURE_R32F:
      return "r32f";
    case OSP_TEXTURE_FORMAT_INVALID:
      return "invalid";
    }

    std::stringstream error;
    error << __FILE__ << ":" << __LINE__ << ": undefined OSPTextureFormat "
          << (int)format;
    throw std::runtime_error(error.str());
  }

  size_t sizeOf(OSPTextureFormat format)
  {
    switch (format) {
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
      case OSP_TEXTURE_FORMAT_INVALID: return 0;
    }

    std::stringstream error;
    error << __FILE__ << ":" << __LINE__ << ": unknown OSPTextureFormat "
          << (int)format;
    throw std::runtime_error(error.str());
  }

  uint32_t logLevel()
  {
    return ospray::api::Device::current->logLevel;
  }

  OSPError loadLocalModule(const std::string &name)
  {
    std::string libName = "ospray_module_" + name;
    loadLibrary(libName, false);

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
      device.lastErrorMsg  = "#ospray: " + message;

      device.error_fcn(e, message.c_str());
    } else {
      // NOTE: No device, but something should still get printed for the user to
      //       debug the calling application.
      std::cerr << "#ospray: INITIALIZATION ERROR --> " << message << std::endl;
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

  OSPTYPEFOR_DEFINITION(void *);
  OSPTYPEFOR_DEFINITION(char *);
  OSPTYPEFOR_DEFINITION(const char *);
  OSPTYPEFOR_DEFINITION(const char []);
  OSPTYPEFOR_DEFINITION(bool);
  OSPTYPEFOR_DEFINITION(char);
  OSPTYPEFOR_DEFINITION(unsigned char);
  OSPTYPEFOR_DEFINITION(vec2uc);
  OSPTYPEFOR_DEFINITION(vec3uc);
  OSPTYPEFOR_DEFINITION(vec4uc);
  OSPTYPEFOR_DEFINITION(short);
  OSPTYPEFOR_DEFINITION(unsigned short);
  OSPTYPEFOR_DEFINITION(int32_t);
  OSPTYPEFOR_DEFINITION(vec2i);
  OSPTYPEFOR_DEFINITION(vec3i);
  OSPTYPEFOR_DEFINITION(vec4i);
  OSPTYPEFOR_DEFINITION(uint32_t);
  OSPTYPEFOR_DEFINITION(vec2ui);
  OSPTYPEFOR_DEFINITION(vec3ui);
  OSPTYPEFOR_DEFINITION(vec4ui);
  OSPTYPEFOR_DEFINITION(int64_t);
  OSPTYPEFOR_DEFINITION(vec2l);
  OSPTYPEFOR_DEFINITION(vec3l);
  OSPTYPEFOR_DEFINITION(vec4l);
  OSPTYPEFOR_DEFINITION(uint64_t);
  OSPTYPEFOR_DEFINITION(vec2ul);
  OSPTYPEFOR_DEFINITION(vec3ul);
  OSPTYPEFOR_DEFINITION(vec4ul);
  OSPTYPEFOR_DEFINITION(float);
  OSPTYPEFOR_DEFINITION(vec2f);
  OSPTYPEFOR_DEFINITION(vec3f);
  OSPTYPEFOR_DEFINITION(vec4f);
  OSPTYPEFOR_DEFINITION(double);
  OSPTYPEFOR_DEFINITION(box1i);
  OSPTYPEFOR_DEFINITION(box2i);
  OSPTYPEFOR_DEFINITION(box3i);
  OSPTYPEFOR_DEFINITION(box4i);
  OSPTYPEFOR_DEFINITION(box1f);
  OSPTYPEFOR_DEFINITION(box2f);
  OSPTYPEFOR_DEFINITION(box3f);
  OSPTYPEFOR_DEFINITION(box4f);
  OSPTYPEFOR_DEFINITION(linear2f);
  OSPTYPEFOR_DEFINITION(linear3f);
  OSPTYPEFOR_DEFINITION(affine2f);
  OSPTYPEFOR_DEFINITION(affine3f);

  OSPTYPEFOR_DEFINITION(OSPObject);
  OSPTYPEFOR_DEFINITION(OSPCamera);
  OSPTYPEFOR_DEFINITION(OSPData);
  OSPTYPEFOR_DEFINITION(OSPFrameBuffer);
  OSPTYPEFOR_DEFINITION(OSPFuture);
  OSPTYPEFOR_DEFINITION(OSPGeometricModel);
  OSPTYPEFOR_DEFINITION(OSPGeometry);
  OSPTYPEFOR_DEFINITION(OSPGroup);
  OSPTYPEFOR_DEFINITION(OSPImageOperation);
  OSPTYPEFOR_DEFINITION(OSPInstance);
  OSPTYPEFOR_DEFINITION(OSPLight);
  OSPTYPEFOR_DEFINITION(OSPMaterial);
  OSPTYPEFOR_DEFINITION(OSPRenderer);
  OSPTYPEFOR_DEFINITION(OSPTexture);
  OSPTYPEFOR_DEFINITION(OSPTransferFunction);
  OSPTYPEFOR_DEFINITION(OSPVolume);
  OSPTYPEFOR_DEFINITION(OSPVolumetricModel);
  OSPTYPEFOR_DEFINITION(OSPWorld);

} // ::ospray

