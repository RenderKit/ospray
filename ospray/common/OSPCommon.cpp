// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// must be first
#define OSPRAY_RKCOMMON_DEFINITIONS
#include "ospray/ospray_cpp/ext/rkcommon.h"

#include "OSPCommon.h"
#include "api/Device.h"

#include "rkcommon/containers/AlignedVector.h"
#include "rkcommon/utility/StringManip.h"

#include <cassert>
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
  alignedFree(ptr);
}

namespace {

using TLSPool = containers::AlignedVector<uint8_t>;
thread_local std::vector<TLSPool> tlsStack;
thread_local size_t topIndex = 0; // TLS stack top index
} // namespace

extern "C" void *pushTLS(size_t size)
{
  // Grow stack
  topIndex++;
  if (topIndex > tlsStack.size())
    tlsStack.resize(topIndex);

  // Grow pool
  TLSPool &tp = tlsStack[topIndex - 1];
  tp.resize(size);
  return tp.data();
}

extern "C" void *reallocTLS(void *ptr, size_t size)
{
  // Silence 'unused variable' warning
  (void)ptr;

  // Grow pool
  TLSPool &tp = tlsStack[topIndex - 1];
  assert(ptr == tp.data());
  tp.resize(size);
  return tp.data();
}

extern "C" void popTLS(void *ptr)
{
  // Silence 'unused variable' warning
  (void)ptr;

  // Lower stack pointer
  assert(topIndex > 0);
  assert(ptr == tlsStack[topIndex - 1].data());
  topIndex--;
}

WarnOnce::WarnOnce(const std::string &s, uint32_t postAtLogLevel) : s(s)
{
  postStatusMsg(postAtLogLevel)
      << "Warning: " << s << " (only reporting first occurrence)";
}

std::string getArgString(const std::string &s)
{
  std::vector<std::string> tokens = rkcommon::utility::split(s, '=');
  if (tokens.size() < 2) {
    std::stringstream ss;
    ss << "Invalid format for command-line argument " << s
       << ". Should be formatted --osp:<parameter>=<value>";
    postStatusMsg(ss, OSP_LOG_WARNING);
    return "";
  } else {
    return tokens.back();
  }
}

int getArgInt(const std::string &s)
{
  std::string value = getArgString(s);
  try {
    return std::stoi(value);
  } catch (...) { // std::invalid_argument or std::out_of_range
    std::stringstream ss;
    ss << "Invalid value '" << value << "' in command-line argument " << s
       << ". Should be an integer";
    postStatusMsg(ss, OSP_LOG_WARNING);
    return 0;
  }
}

void initFromCommandLine(int *_ac, const char ***_av)
{
  using namespace rkcommon::utility;

  auto &device = ospray::api::Device::current;

  if (_ac && _av) {
    int &ac = *_ac;
    auto &av = *_av;

    // If we have any device-params those must be set first to avoid
    // initializing the device in an invalid or partial state
    bool hadDeviceParams = false;
    for (int i = 1; i < ac;) {
      if (std::strncmp(av[i], "--osp:device-params", 19) == 0) {
        hadDeviceParams = true;
        std::string parmesan = getArgString(av[i]);
        std::vector<std::string> parmList = split(parmesan, ',');
        for (std::string &p : parmList) {
          std::vector<std::string> kv = split(p, ':');
          if (kv.size() != 2) {
            postStatusMsg(
                "Invalid parameters provided for --osp:device-params. "
                "Must be formatted as <param>:<value>[,<param>:<value>,...]");
          } else {
            device->setParam(kv[0], kv[1]);
          }
        }
        removeArgs(ac, av, i, 1);
      } else {
        ++i;
      }
    }
    if (hadDeviceParams) {
      device->commit();
    }

    for (int i = 1; i < ac;) {
      std::string parm = av[i];
      // flag-style arguments
      if (parm == "--osp:debug") {
        device->setParam("debug", true);
        device->setParam("logOutput", std::string("cout"));
        device->setParam("errorOutput", std::string("cerr"));
        removeArgs(ac, av, i, 1);
      } else if (parm == "--osp:warn-as-error") {
        device->setParam("warnAsError", true);
        removeArgs(ac, av, i, 1);
      } else if (parm == "--osp:verbose") {
        device->setParam("logLevel", OSP_LOG_INFO);
        device->setParam("logOutput", std::string("cout"));
        device->setParam("errorOutput", std::string("cerr"));
        removeArgs(ac, av, i, 1);
      } else if (parm == "--osp:vv") {
        device->setParam("logLevel", OSP_LOG_DEBUG);
        device->setParam("logOutput", std::string("cout"));
        device->setParam("errorOutput", std::string("cerr"));
        removeArgs(ac, av, i, 1);
      }
      // arguments taking required values
      else if (beginsWith(parm, "--osp:log-level")) {
        std::string str = getArgString(parm);
        auto level = api::Device::logLevelFromString(str);
        device->setParam("logLevel", level.value_or(0));
        removeArgs(ac, av, i, 1);
      } else if (beginsWith(parm, "--osp:log-output")) {
        std::string dst = getArgString(parm);
        if (dst == "cout" || dst == "cerr") {
          device->setParam("logOutput", dst);
        } else {
          std::stringstream ss;
          ss << "Invalid output '" << dst
             << "' provided for --osp:log-output. Must be 'cerr' or 'cout'";
          postStatusMsg(ss);
        }
        removeArgs(ac, av, i, 1);
      } else if (beginsWith(parm, "--osp:error-output")) {
        std::string dst = getArgString(parm);
        if (dst == "cout" || dst == "cerr") {
          device->setParam("errorOutput", dst);
        } else {
          std::stringstream ss;
          ss << "Invalid output '" << dst
             << "' provided for --osp:error-output. Must be 'cerr' or 'cout'";
          postStatusMsg(ss);
        }
        removeArgs(ac, av, i, 1);
      } else if (beginsWith(parm, "--osp:num-threads")) {
        int nt = std::max(1, getArgInt(parm));
        device->setParam("numThreads", nt);
        removeArgs(ac, av, i, 1);
      } else if (beginsWith(parm, "--osp:set-affinity")) {
        int val = getArgInt(parm);
        if (val == 0 || val == 1) {
          device->setParam<int>("setAffinity", val);
        } else {
          postStatusMsg(
              "Invalid value provided for --osp:set-affinity. "
              "Must be 0 or 1");
        }
        removeArgs(ac, av, i, 1);
      } else {
        // silently ignore other arguments
        i++;
      }

      // ALOK: apply changes so that error messages and warnings during
      // initialization might appear for the user. Without this, any errors in
      // parsing will never be shown, even if the user sets the log output
      // stream, log level, etc. They still may not be shown if the output is
      // not set, however
      device->commit();
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
  case OSP_STRING:
    return sizeof(void *);
  case OSP_BOOL:
    return sizeof(bool);
  case OSP_CHAR:
    return sizeof(int8);
  case OSP_UCHAR:
    return sizeof(uint8);
  case OSP_VEC2UC:
    return sizeof(vec2uc);
  case OSP_VEC3UC:
    return sizeof(vec3uc);
  case OSP_VEC4UC:
    return sizeof(vec4uc);
  case OSP_VEC2C:
    return sizeof(vec2c);
  case OSP_VEC3C:
    return sizeof(vec3c);
  case OSP_VEC4C:
    return sizeof(vec4c);
  case OSP_SHORT:
    return sizeof(int16);
  case OSP_VEC2S:
    return sizeof(vec2s);
  case OSP_VEC3S:
    return sizeof(vec3s);
  case OSP_VEC4S:
    return sizeof(vec4s);
  case OSP_USHORT:
    return sizeof(uint16);
  case OSP_VEC2US:
    return sizeof(vec2us);
  case OSP_VEC3US:
    return sizeof(vec3us);
  case OSP_VEC4US:
    return sizeof(vec4us);
  case OSP_INT:
    return sizeof(int32);
  case OSP_VEC2I:
    return sizeof(vec2i);
  case OSP_VEC3I:
    return sizeof(vec3i);
  case OSP_VEC4I:
    return sizeof(vec4i);
  case OSP_UINT:
    return sizeof(uint32);
  case OSP_VEC2UI:
    return sizeof(vec2ui);
  case OSP_VEC3UI:
    return sizeof(vec3ui);
  case OSP_VEC4UI:
    return sizeof(vec4ui);
  case OSP_LONG:
    return sizeof(int64);
  case OSP_VEC2L:
    return sizeof(vec2l);
  case OSP_VEC3L:
    return sizeof(vec3l);
  case OSP_VEC4L:
    return sizeof(vec4l);
  case OSP_ULONG:
    return sizeof(uint64);
  case OSP_VEC2UL:
    return sizeof(vec2ul);
  case OSP_VEC3UL:
    return sizeof(vec3ul);
  case OSP_VEC4UL:
    return sizeof(vec4ul);
  case OSP_FLOAT:
    return sizeof(float);
  case OSP_VEC2F:
    return sizeof(vec2f);
  case OSP_VEC3F:
    return sizeof(vec3f);
  case OSP_VEC4F:
    return sizeof(vec4f);
  case OSP_DOUBLE:
    return sizeof(double);
  case OSP_VEC2D:
    return sizeof(vec2d);
  case OSP_VEC3D:
    return sizeof(vec3d);
  case OSP_VEC4D:
    return sizeof(vec4d);
  case OSP_BOX1I:
    return sizeof(box1i);
  case OSP_BOX2I:
    return sizeof(box2i);
  case OSP_BOX3I:
    return sizeof(box3i);
  case OSP_BOX4I:
    return sizeof(box4i);
  case OSP_BOX1F:
    return sizeof(box1f);
  case OSP_BOX2F:
    return sizeof(box2f);
  case OSP_BOX3F:
    return sizeof(box3f);
  case OSP_BOX4F:
    return sizeof(box4f);
  case OSP_LINEAR2F:
    return sizeof(linear2f);
  case OSP_LINEAR3F:
    return sizeof(linear3f);
  case OSP_AFFINE2F:
    return sizeof(affine2f);
  case OSP_AFFINE3F:
    return sizeof(affine3f);
  case OSP_UNKNOWN:
    return 0;
  }

  std::stringstream error;
  error << __FILE__ << ":" << __LINE__ << ": unknown OSPDataType " << (int)type;
  throw std::runtime_error(error.str());
}

OSPDataType typeOf(const char *string)
{
  if (string == nullptr)
    return (OSP_UNKNOWN);
  if (strcmp(string, "bool") == 0)
    return (OSP_BOOL);
  if (strcmp(string, "char") == 0)
    return (OSP_CHAR);
  if (strcmp(string, "double") == 0)
    return (OSP_DOUBLE);
  if (strcmp(string, "float") == 0)
    return (OSP_FLOAT);
  if (strcmp(string, "vec2f") == 0)
    return (OSP_VEC2F);
  if (strcmp(string, "vec3f") == 0)
    return (OSP_VEC3F);
  if (strcmp(string, "vec4f") == 0)
    return (OSP_VEC4F);
  if (strcmp(string, "vec2d") == 0)
    return (OSP_VEC2D);
  if (strcmp(string, "vec3d") == 0)
    return (OSP_VEC3D);
  if (strcmp(string, "vec4d") == 0)
    return (OSP_VEC4D);
  if (strcmp(string, "int") == 0)
    return (OSP_INT);
  if (strcmp(string, "vec2i") == 0)
    return (OSP_VEC2I);
  if (strcmp(string, "vec3i") == 0)
    return (OSP_VEC3I);
  if (strcmp(string, "vec4i") == 0)
    return (OSP_VEC4I);
  if (strcmp(string, "uchar") == 0)
    return (OSP_UCHAR);
  if (strcmp(string, "vec2uc") == 0)
    return (OSP_VEC2UC);
  if (strcmp(string, "vec3uc") == 0)
    return (OSP_VEC3UC);
  if (strcmp(string, "vec4uc") == 0)
    return (OSP_VEC4UC);
  if (strcmp(string, "vec2c") == 0)
    return (OSP_VEC2C);
  if (strcmp(string, "vec3c") == 0)
    return (OSP_VEC3C);
  if (strcmp(string, "vec4c") == 0)
    return (OSP_VEC4C);
  if (strcmp(string, "short") == 0)
    return (OSP_SHORT);
  if (strcmp(string, "vec2s") == 0)
    return (OSP_VEC2S);
  if (strcmp(string, "vec3s") == 0)
    return (OSP_VEC3S);
  if (strcmp(string, "vec4s") == 0)
    return (OSP_VEC4S);
  if (strcmp(string, "ushort") == 0)
    return (OSP_USHORT);
  if (strcmp(string, "vec2us") == 0)
    return (OSP_VEC2US);
  if (strcmp(string, "vec3us") == 0)
    return (OSP_VEC3US);
  if (strcmp(string, "vec4us") == 0)
    return (OSP_VEC4US);
  if (strcmp(string, "uint") == 0)
    return (OSP_UINT);
  if (strcmp(string, "uint2") == 0)
    return (OSP_VEC2UI);
  if (strcmp(string, "uint3") == 0)
    return (OSP_VEC3UI);
  if (strcmp(string, "uint4") == 0)
    return (OSP_VEC4UI);
  return (OSP_UNKNOWN);
}

std::string stringFor(OSPDataType type)
{
  switch (type) {
  case OSP_VOID_PTR:
    return "void_ptr";
  case OSP_OBJECT:
    return "object";
  case OSP_CAMERA:
    return "camera";
  case OSP_DATA:
    return "data";
  case OSP_DEVICE:
    return "device";
  case OSP_FRAMEBUFFER:
    return "framebuffer";
  case OSP_FUTURE:
    return "future";
  case OSP_GEOMETRIC_MODEL:
    return "geometric_model";
  case OSP_GEOMETRY:
    return "geometry";
  case OSP_GROUP:
    return "group";
  case OSP_IMAGE_OPERATION:
    return "image_operation";
  case OSP_INSTANCE:
    return "instance";
  case OSP_LIGHT:
    return "light";
  case OSP_MATERIAL:
    return "material";
  case OSP_RENDERER:
    return "renderer";
  case OSP_TEXTURE:
    return "texture";
  case OSP_TRANSFER_FUNCTION:
    return "transfer_function";
  case OSP_VOLUME:
    return "volume";
  case OSP_VOLUMETRIC_MODEL:
    return "volumetric_model";
  case OSP_WORLD:
    return "world";
  case OSP_STRING:
    return "string";
  case OSP_BOOL:
    return "bool";
  case OSP_CHAR:
    return "char";
  case OSP_UCHAR:
    return "uchar";
  case OSP_VEC2UC:
    return "vec2uc";
  case OSP_VEC3UC:
    return "vec3uc";
  case OSP_VEC4UC:
    return "vec4uc";
  case OSP_VEC2C:
    return "vec2c";
  case OSP_VEC3C:
    return "vec3c";
  case OSP_VEC4C:
    return "vec4c";
  case OSP_SHORT:
    return "short";
  case OSP_VEC2S:
    return "vec2s";
  case OSP_VEC3S:
    return "vec3s";
  case OSP_VEC4S:
    return "vec4s";
  case OSP_USHORT:
    return "ushort";
  case OSP_VEC2US:
    return "vec2us";
  case OSP_VEC3US:
    return "vec3us";
  case OSP_VEC4US:
    return "vec4us";
  case OSP_INT:
    return "int";
  case OSP_VEC2I:
    return "vec2i";
  case OSP_VEC3I:
    return "vec3i";
  case OSP_VEC4I:
    return "vec4i";
  case OSP_UINT:
    return "uint";
  case OSP_VEC2UI:
    return "vec2ui";
  case OSP_VEC3UI:
    return "vec3ui";
  case OSP_VEC4UI:
    return "vec4ui";
  case OSP_LONG:
    return "long";
  case OSP_VEC2L:
    return "vec2l";
  case OSP_VEC3L:
    return "vec3l";
  case OSP_VEC4L:
    return "vec4l";
  case OSP_ULONG:
    return "ulong";
  case OSP_VEC2UL:
    return "vec2ul";
  case OSP_VEC3UL:
    return "vec3ul";
  case OSP_VEC4UL:
    return "vec4ul";
  case OSP_FLOAT:
    return "float";
  case OSP_VEC2F:
    return "vec2f";
  case OSP_VEC3F:
    return "vec3f";
  case OSP_VEC4F:
    return "vec4f";
  case OSP_VEC2D:
    return "vec2d";
  case OSP_VEC3D:
    return "vec3d";
  case OSP_VEC4D:
    return "vec4d";
  case OSP_DOUBLE:
    return "double";
  case OSP_BOX1I:
    return "box1i";
  case OSP_BOX2I:
    return "box2i";
  case OSP_BOX3I:
    return "box3i";
  case OSP_BOX4I:
    return "box4i";
  case OSP_BOX1F:
    return "box1f";
  case OSP_BOX2F:
    return "box2f";
  case OSP_BOX3F:
    return "box3f";
  case OSP_BOX4F:
    return "box4f";
  case OSP_LINEAR2F:
    return "linear2f";
  case OSP_LINEAR3F:
    return "linear3f";
  case OSP_AFFINE2F:
    return "affine2f";
  case OSP_AFFINE3F:
    return "affine3f";
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
  case OSP_TEXTURE_RGBA16:
    return "rgba16";
  case OSP_TEXTURE_RGB16:
    return "rgb16";
  case OSP_TEXTURE_RA16:
    return "ra16";
  case OSP_TEXTURE_R16:
    return "r16";
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
  case OSP_TEXTURE_SRGBA:
    return sizeof(uint32);
  case OSP_TEXTURE_RGBA32F:
    return sizeof(vec4f);
  case OSP_TEXTURE_RGB8:
  case OSP_TEXTURE_SRGB:
    return sizeof(vec3uc);
  case OSP_TEXTURE_RGB32F:
    return sizeof(vec3f);
  case OSP_TEXTURE_R8:
  case OSP_TEXTURE_L8:
    return sizeof(uint8);
  case OSP_TEXTURE_RA8:
  case OSP_TEXTURE_LA8:
    return sizeof(vec2uc);
  case OSP_TEXTURE_R32F:
    return sizeof(float);
  case OSP_TEXTURE_RGBA16:
    return sizeof(vec4us);
  case OSP_TEXTURE_RGB16:
    return sizeof(vec3us);
  case OSP_TEXTURE_RA16:
    return sizeof(vec2us);
  case OSP_TEXTURE_R16:
    return sizeof(uint16);
  case OSP_TEXTURE_FORMAT_INVALID:
    return 0;
  }

  std::stringstream error;
  error << __FILE__ << ":" << __LINE__ << ": unknown OSPTextureFormat "
        << (int)format;
  throw std::runtime_error(error.str());
}

size_t sizeOf(OSPFrameBufferFormat format)
{
  size_t bytes = 0;

  switch (format) {
  case OSP_FB_RGBA8:
  case OSP_FB_SRGBA:
    bytes = sizeof(uint32_t);
    break;
  case OSP_FB_RGBA32F:
    bytes = sizeof(vec4f);
    break;
  default:
    break;
  }

  return bytes;
}

uint32_t logLevel()
{
  return ospray::api::Device::current->logLevel;
}

OSPError loadLocalModule(const std::string &name)
{
  std::string libName = "ospray_module_" + name;
  try {
    loadLibrary(libName, false);
  } catch (const std::exception &e) {
    handleError(OSP_INVALID_OPERATION, e.what());
    return OSP_INVALID_OPERATION;
  } catch (...) {
    return OSP_INVALID_OPERATION;
  }

  std::string initSymName = "ospray_module_init_" + name;
  void *initSym = getSymbol(initSymName);
  if (!initSym) {
    handleError(OSP_INVALID_OPERATION,
        "Could not find module initializer " + initSymName);
    unloadLibrary(libName);
    return OSP_INVALID_OPERATION;
  }

  auto initMethod = (OSPError(*)(int16_t, int16_t, int16_t))initSym;
  auto err = initMethod(
      OSPRAY_VERSION_MAJOR, OSPRAY_VERSION_MINOR, OSPRAY_VERSION_PATCH);

  if (err != OSP_NO_ERROR) {
    handleError(err, "Initialization of module " + name + " failed");
    unloadLibrary(libName);
  }

  return err;
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
  if (!api::deviceIsSet())
    return;

  auto level = logLevel();
  if (level == OSP_LOG_NONE)
    return;

  auto &device = *api::Device::current;

  if (level <= postAtLogLevel) {
    bool logAsError =
        (device.warningsAreErrors && postAtLogLevel == OSP_LOG_WARNING)
        || postAtLogLevel == OSP_LOG_ERROR;

    if (logAsError)
      handleError(OSP_UNKNOWN_ERROR, msg + '\n');
    else
      api::Device::current->msg_fcn(
          api::Device::current->statusUserData, (msg + '\n').c_str());
  }
}

void handleError(OSPError e, const std::string &message)
{
  if (api::deviceIsSet()) {
    auto &device = api::currentDevice();

    device.lastErrorCode = e;
    device.lastErrorMsg = "#ospray: " + message;

    device.error_fcn(device.errorUserData, e, message.c_str());
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
OSPTYPEFOR_DEFINITION(const char[]);
OSPTYPEFOR_DEFINITION(bool);
OSPTYPEFOR_DEFINITION(char);
OSPTYPEFOR_DEFINITION(unsigned char);
OSPTYPEFOR_DEFINITION(short);
OSPTYPEFOR_DEFINITION(unsigned short);
OSPTYPEFOR_DEFINITION(int);
OSPTYPEFOR_DEFINITION(unsigned int);
OSPTYPEFOR_DEFINITION(long);
OSPTYPEFOR_DEFINITION(unsigned long);
OSPTYPEFOR_DEFINITION(long long);
OSPTYPEFOR_DEFINITION(unsigned long long);
OSPTYPEFOR_DEFINITION(float);
OSPTYPEFOR_DEFINITION(double);

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

OSPDIMENSIONALITYOF_DEFINITION(OSP_VEC2UC);
OSPDIMENSIONALITYOF_DEFINITION(OSP_VEC3UC);
OSPDIMENSIONALITYOF_DEFINITION(OSP_VEC4UC);
OSPDIMENSIONALITYOF_DEFINITION(OSP_VEC2C);
OSPDIMENSIONALITYOF_DEFINITION(OSP_VEC3C);
OSPDIMENSIONALITYOF_DEFINITION(OSP_VEC4C);
OSPDIMENSIONALITYOF_DEFINITION(OSP_VEC2I);
OSPDIMENSIONALITYOF_DEFINITION(OSP_VEC3I);
OSPDIMENSIONALITYOF_DEFINITION(OSP_VEC4I);
OSPDIMENSIONALITYOF_DEFINITION(OSP_VEC2UI);
OSPDIMENSIONALITYOF_DEFINITION(OSP_VEC3UI);
OSPDIMENSIONALITYOF_DEFINITION(OSP_VEC4UI);
OSPDIMENSIONALITYOF_DEFINITION(OSP_VEC2L);
OSPDIMENSIONALITYOF_DEFINITION(OSP_VEC3L);
OSPDIMENSIONALITYOF_DEFINITION(OSP_VEC4L);
OSPDIMENSIONALITYOF_DEFINITION(OSP_VEC2UL);
OSPDIMENSIONALITYOF_DEFINITION(OSP_VEC3UL);
OSPDIMENSIONALITYOF_DEFINITION(OSP_VEC4UL);
OSPDIMENSIONALITYOF_DEFINITION(OSP_VEC2F);
OSPDIMENSIONALITYOF_DEFINITION(OSP_VEC3F);
OSPDIMENSIONALITYOF_DEFINITION(OSP_VEC4F);
OSPDIMENSIONALITYOF_DEFINITION(OSP_VEC2D);
OSPDIMENSIONALITYOF_DEFINITION(OSP_VEC3D);
OSPDIMENSIONALITYOF_DEFINITION(OSP_VEC4D);
OSPDIMENSIONALITYOF_DEFINITION(OSP_BOX1I);
OSPDIMENSIONALITYOF_DEFINITION(OSP_BOX2I);
OSPDIMENSIONALITYOF_DEFINITION(OSP_BOX3I);
OSPDIMENSIONALITYOF_DEFINITION(OSP_BOX4I);
OSPDIMENSIONALITYOF_DEFINITION(OSP_BOX1F);
OSPDIMENSIONALITYOF_DEFINITION(OSP_BOX2F);
OSPDIMENSIONALITYOF_DEFINITION(OSP_BOX3F);
OSPDIMENSIONALITYOF_DEFINITION(OSP_BOX4F);

} // namespace ospray
