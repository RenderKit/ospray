// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "Device.h"
// ospcommon
#include "ospcommon/os/library.h"
#include "ospcommon/tasking/tasking_system_init.h"
#include "ospcommon/utility/getEnvVar.h"

#include <map>

namespace ospray {
namespace api {

static FactoryMap<Device> g_devicesMap;

// Helper functions ///////////////////////////////////////////////////////////

template <typename OSTREAM_T>
static inline void installStatusMsgFunc(Device &device, OSTREAM_T &stream)
{
  device.msg_fcn = [&](const char *msg) { stream << msg; };
}

template <typename OSTREAM_T>
static inline void installErrorMsgFunc(Device &device, OSTREAM_T &stream)
{
  device.error_fcn = [&](OSPError e, const char *msg) {
    stream << "OSPRAY ERROR [" << e << "]: " << msg << std::endl;
  };
}

// Device definitions /////////////////////////////////////////////////////////

memory::IntrusivePtr<Device> Device::current;
uint32_t Device::logLevel = OSP_LOG_NONE;

Device *Device::createDevice(const char *type)
{
  // NOTE(jda) - If a user is manually creating the device (i.e. not using
  //             ospInit() to do it), then we need to check if there's a
  //             valid library for core ospray in our main symbol lookup
  //             table.
  auto &repo = *LibraryRepository::getInstance();
  if (!repo.libraryExists("ospray"))
    repo.addDefaultLibrary();

  if (!repo.libraryExists("ospray_module_ispc") && type == std::string("cpu"))
    repo.add("ospray_module_ispc");

  return createInstanceHelper(type, g_devicesMap[type]);
}

void Device::registerType(const char *type, FactoryFcn<Device> f)
{
  g_devicesMap[type] = f;
}

void Device::commit()
{
  auto OSPRAY_DEBUG = utility::getEnvVar<int>("OSPRAY_DEBUG");
  debugMode = OSPRAY_DEBUG.value_or(getParam<bool>("debug", 0));

  auto OSPRAY_WARN = utility::getEnvVar<int>("OSPRAY_WARN_AS_ERROR");
  warningsAreErrors =
      OSPRAY_WARN.value_or(getParam<bool>("warnAsError", false));

  auto OSPRAY_TRACE_API = utility::getEnvVar<int>("OSPRAY_TRACE_API");
  bool traceAPI = OSPRAY_TRACE_API.value_or(getParam<bool>("traceApi", false));

  if (traceAPI && !apiTraceEnabled) {
    auto streamPtr = std::make_shared<std::ofstream>("ospray_api_trace.txt");

    trace_fcn = [=](const char *message) {
      auto &stream = *streamPtr;
      stream << message << std::endl;
    };
  } else if (!traceAPI) {
    trace_fcn = [](const char *) {};
  }

  apiTraceEnabled = traceAPI;

  logLevel = getParam<int>("logLevel", OSP_LOG_NONE);

  auto logLevelStr =
      utility::getEnvVar<std::string>("OSPRAY_LOG_LEVEL").value_or("");

  logLevel = logLevelFromString(logLevelStr).value_or(logLevel);

  auto OSPRAY_NUM_THREADS = utility::getEnvVar<int>("OSPRAY_NUM_THREADS");
  numThreads = OSPRAY_NUM_THREADS.value_or(getParam<int>("numThreads", -1));

  auto OSPRAY_LOG_OUTPUT = utility::getEnvVar<std::string>("OSPRAY_LOG_OUTPUT");

  auto dst = OSPRAY_LOG_OUTPUT.value_or(getParam<std::string>("logOutput", ""));

  if (dst == "cout")
    installStatusMsgFunc(*this, std::cout);
  else if (dst == "cerr")
    installStatusMsgFunc(*this, std::cerr);
  else if (dst == "none")
    msg_fcn = [](const char *) {};

  auto OSPRAY_ERROR_OUTPUT =
      utility::getEnvVar<std::string>("OSPRAY_ERROR_OUTPUT");

  dst = OSPRAY_ERROR_OUTPUT.value_or(getParam<std::string>("errorOutput", ""));

  if (dst == "cout")
    installErrorMsgFunc(*this, std::cout);
  else if (dst == "cerr")
    installErrorMsgFunc(*this, std::cerr);
  else if (dst == "none")
    error_fcn = [](OSPError, const char *) {};

  if (debugMode) {
    logLevel = OSP_LOG_DEBUG;
    numThreads = 1;
    installStatusMsgFunc(*this, std::cout);
    installErrorMsgFunc(*this, std::cerr);
  }

  threadAffinity = AUTO_DETECT;

  auto OSPRAY_SET_AFFINITY = utility::getEnvVar<int>("OSPRAY_SET_AFFINITY");
  if (OSPRAY_SET_AFFINITY)
    threadAffinity = OSPRAY_SET_AFFINITY.value();

  threadAffinity = getParam<bool>("setAffinity", threadAffinity);

  tasking::initTaskingSystem(numThreads);

  committed = true;
}

bool Device::isCommitted()
{
  return committed;
}

bool deviceIsSet()
{
  return Device::current.ptr != nullptr;
}

Device &currentDevice()
{
  return *Device::current;
}

std::string generateEmbreeDeviceCfg(const Device &device)
{
  std::stringstream embreeConfig;

  if (device.debugMode)
    embreeConfig << " verbose=2";

  if (device.threadAffinity == api::Device::AFFINITIZE)
    embreeConfig << ",set_affinity=1";
  else if (device.threadAffinity == api::Device::DEAFFINITIZE)
    embreeConfig << ",set_affinity=0";

  return embreeConfig.str();
}

int64_t Device::getProperty(const OSPDeviceProperty prop)
{
  /* documented properties */
  switch (prop) {
  case OSP_DEVICE_VERSION:
    return 10000 * OSPRAY_VERSION_MAJOR + 100 * OSPRAY_VERSION_MINOR
        + OSPRAY_VERSION_PATCH;
  case OSP_DEVICE_VERSION_MAJOR:
    return OSPRAY_VERSION_MAJOR;
  case OSP_DEVICE_VERSION_MINOR:
    return OSPRAY_VERSION_MINOR;
  case OSP_DEVICE_VERSION_PATCH:
    return OSPRAY_VERSION_PATCH;
  case OSP_DEVICE_SO_VERSION:
    return OSPRAY_SOVERSION;
  default:
    handleError(OSP_INVALID_ARGUMENT, "unknown readable property");
    return 0;
  }
}

utility::Optional<int> Device::logLevelFromString(const std::string &str)
{
  utility::Optional<int> retval;

  if (str == "none")
    retval = OSP_LOG_NONE;
  else if (str == "debug")
    retval = OSP_LOG_DEBUG;
  else if (str == "info")
    retval = OSP_LOG_INFO;
  else if (str == "warning")
    retval = OSP_LOG_WARNING;
  else if (str == "error")
    retval = OSP_LOG_ERROR;

  return retval;
}

} // namespace api

OSPTYPEFOR_DEFINITION(api::Device *);

} // namespace ospray
