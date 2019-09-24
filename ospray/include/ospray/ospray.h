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

#pragma once

#ifndef NULL
# if __cplusplus >= 201103L
#  define NULL nullptr
# else
#  define NULL 0
# endif
#endif

#include <sys/types.h>
#include <stdint.h>

#include "OSPEnums.h"

#ifdef _WIN32
#  ifdef ospray_EXPORTS
#    define OSPRAY_INTERFACE __declspec(dllexport)
#  else
#    define OSPRAY_INTERFACE __declspec(dllimport)
#  endif
#else
#  define OSPRAY_INTERFACE
#endif

#ifdef __GNUC__
#define OSP_DEPRECATED __attribute__((deprecated))
#elif defined(_MSC_VER)
#define OSP_DEPRECATED __declspec(deprecated)
#else
#define OSP_DEPRECATED
#endif

#ifdef __cplusplus
// C++ DOES support default initializers
#define OSP_DEFAULT_VAL(a) = a
#else
/* C99 does NOT support default initializers, so we use this macro
   to define them away */
#define OSP_DEFAULT_VAL(a)
#endif

// Give OSPRay handle types a concrete defintion to enable C++ type checking
#ifdef __cplusplus
namespace osp {
  struct Device;
  struct ManagedObject    {};
  struct Camera           : public ManagedObject {};
  struct Data             : public ManagedObject {};
  struct FrameBuffer      : public ManagedObject {};
  struct Future           : public ManagedObject {};
  struct GeometricModel   : public ManagedObject {};
  struct Geometry         : public ManagedObject {};
  struct Group            : public ManagedObject {};
  struct ImageOperation   : public ManagedObject {};
  struct Instance         : public ManagedObject {};
  struct Light            : public ManagedObject {};
  struct Material         : public ManagedObject {};
  struct Renderer         : public ManagedObject {};
  struct Texture          : public ManagedObject {};
  struct TransferFunction : public ManagedObject {};
  struct Volume           : public ManagedObject {};
  struct VolumetricModel  : public ManagedObject {};
  struct World            : public ManagedObject {};
}

typedef osp::Device            *OSPDevice;
typedef osp::Camera            *OSPCamera;
typedef osp::Data              *OSPData;
typedef osp::FrameBuffer       *OSPFrameBuffer;
typedef osp::Future            *OSPFuture;
typedef osp::GeometricModel    *OSPGeometricModel;
typedef osp::Geometry          *OSPGeometry;
typedef osp::Group             *OSPGroup;
typedef osp::ImageOperation    *OSPImageOperation;
typedef osp::Instance          *OSPInstance;
typedef osp::Light             *OSPLight;
typedef osp::ManagedObject     *OSPObject;
typedef osp::Material          *OSPMaterial;
typedef osp::Renderer          *OSPRenderer;
typedef osp::Texture           *OSPTexture;
typedef osp::TransferFunction  *OSPTransferFunction;
typedef osp::Volume            *OSPVolume;
typedef osp::VolumetricModel   *OSPVolumetricModel;
typedef osp::World             *OSPWorld;
#else
typedef void *OSPDevice;
typedef void _OSPManagedObject;

/* Abstract object types. in C99, those are all the same because C99
   doesn't know inheritance, and we want to make sure that a
   OSPGeometry can still be passed to a function that expects a
   OSPObject, etc. */
typedef _OSPManagedObject *OSPManagedObject,
  *OSPCamera,
  *OSPData,
  *OSPFrameBuffer,
  *OSPFuture,
  *OSPGeometricModel,
  *OSPGeometry,
  *OSPGroup,
  *OSPImageOperation,
  *OSPInstance,
  *OSPLight,
  *OSPMaterial,
  *OSPObject,
  *OSPRenderer,
  *OSPTexture,
  *OSPTransferFunction,
  *OSPVolume,
  *OSPVolumetricModel,
  *OSPWorld;
#endif

#ifdef __cplusplus
extern "C" {
#endif

  // OSPRay Initialization ////////////////////////////////////////////////////

  /* Initialize the OSPRay engine (for single-node user application) using
     commandline arguments...equivalent to doing ospNewDevice() followed by
     ospSetCurrentDevice()

     returns OSPError value to report any errors during initialization */
  OSPRAY_INTERFACE OSPError ospInit(int *argc, const char **argv);

  // returns the OSPRay Version in use by the device
  OSPRAY_INTERFACE int64_t ospDeviceGetProperty(OSPDevice, OSPDeviceProperty);

  // Shutdown the OSPRay engine...effectively deletes whatever device is currently set.
  OSPRAY_INTERFACE void ospShutdown();

  // Create an OSPRay engine backend using explicit device string.
  OSPRAY_INTERFACE OSPDevice ospNewDevice(const char *deviceType OSP_DEFAULT_VAL("default"));

  // Set current device the API responds to
  OSPRAY_INTERFACE void ospSetCurrentDevice(OSPDevice);

  // Get the currently set device
  OSPRAY_INTERFACE OSPDevice ospGetCurrentDevice();

  OSPRAY_INTERFACE void ospDeviceSetString(OSPDevice, const char *id, const char *s);
  OSPRAY_INTERFACE void ospDeviceSetBool(OSPDevice, const char *id, int x);
  OSPRAY_INTERFACE void ospDeviceSetInt(OSPDevice, const char *id, int x);
  OSPRAY_INTERFACE void ospDeviceSetVoidPtr(OSPDevice, const char *id, void *v);

  // Status message callback function type
  typedef void (*OSPStatusFunc)(const char* messageText);

  // Set callback for given Device to call when a status message occurs
  OSPRAY_INTERFACE void ospDeviceSetStatusFunc(OSPDevice, OSPStatusFunc);

  // Error message callback function type
  typedef void (*OSPErrorFunc)(OSPError, const char* errorDetails);

  // Set callback for given Device to call when an error occurs
  OSPRAY_INTERFACE void ospDeviceSetErrorFunc(OSPDevice, OSPErrorFunc);

  // Get the OSPError code for the last error that has occurred on the device
  OSPRAY_INTERFACE OSPError ospDeviceGetLastErrorCode(OSPDevice);

  // Get the message for the last error that has occurred on the device
  OSPRAY_INTERFACE const char* ospDeviceGetLastErrorMsg(OSPDevice);

  // Commit parameters on a given device
  OSPRAY_INTERFACE void ospDeviceCommit(OSPDevice);

  /* Load module 'name' from shard lib libospray_module_<name>.so

     returns OSPError value to report any errors during initialization */
  OSPRAY_INTERFACE OSPError ospLoadModule(const char *name);

  // OSPRay Data Arrays ///////////////////////////////////////////////////////

  OSPRAY_INTERFACE OSPData ospNewSharedData(const void *sharedData,
      OSPDataType,
      uint32_t numItems1,
      int64_t byteStride1 OSP_DEFAULT_VAL(0),
      uint32_t numItems2 OSP_DEFAULT_VAL(1),
      int64_t byteStride2 OSP_DEFAULT_VAL(0),
      uint32_t numItems3 OSP_DEFAULT_VAL(1),
      int64_t byteStride3 OSP_DEFAULT_VAL(0));

  OSPRAY_INTERFACE OSPData ospNewData(OSPDataType,
      uint32_t numItems1,
      uint32_t numItems2 OSP_DEFAULT_VAL(1),
      uint32_t numItems3 OSP_DEFAULT_VAL(1));

  OSPRAY_INTERFACE void ospCopyData(const OSPData source,
      OSPData destination,
      uint32_t destinationIndex1 OSP_DEFAULT_VAL(0),
      uint32_t destinationIndex2 OSP_DEFAULT_VAL(0),
      uint32_t destinationIndex3 OSP_DEFAULT_VAL(0));

  // Renderable Objects ///////////////////////////////////////////////////////

  OSPRAY_INTERFACE OSPLight ospNewLight(const char *type);

  OSPRAY_INTERFACE OSPCamera ospNewCamera(const char *type);

  OSPRAY_INTERFACE OSPGeometry ospNewGeometry(const char *type);
  OSPRAY_INTERFACE OSPVolume ospNewVolume(const char *type);

  OSPRAY_INTERFACE OSPGeometricModel ospNewGeometricModel(OSPGeometry);
  OSPRAY_INTERFACE OSPVolumetricModel ospNewVolumetricModel(OSPVolume);

  // Model Meta-Data //////////////////////////////////////////////////////////

  OSPRAY_INTERFACE OSPMaterial ospNewMaterial(const char *rendererType,
                                              const char *materialType);

  OSPRAY_INTERFACE OSPTransferFunction ospNewTransferFunction(const char *type);

  OSPRAY_INTERFACE OSPTexture ospNewTexture(const char *type);

  // Instancing ///////////////////////////////////////////////////////////////

  OSPRAY_INTERFACE OSPGroup ospNewGroup();
  OSPRAY_INTERFACE OSPInstance ospNewInstance(OSPGroup);

  // Top-level Worlds /////////////////////////////////////////////////////////

  OSPRAY_INTERFACE OSPWorld ospNewWorld();

  // Object Parameters ////////////////////////////////////////////////////////

  OSPRAY_INTERFACE void ospSetString(OSPObject, const char *id, const char *s);

  OSPRAY_INTERFACE void ospSetObject(OSPObject, const char *id, OSPObject other);
  OSPRAY_INTERFACE void ospSetData(OSPObject, const char *id, OSPData);

  OSPRAY_INTERFACE void ospSetBool(OSPObject, const char *id, int x);
  OSPRAY_INTERFACE void ospSetFloat(OSPObject, const char *id, float x);
  OSPRAY_INTERFACE void ospSetInt(OSPObject, const char *id, int x);

  OSPRAY_INTERFACE void ospSetVec2f(OSPObject, const char *id, float x, float y);
  OSPRAY_INTERFACE void ospSetVec2fv(OSPObject, const char *id, const float *xy);
  OSPRAY_INTERFACE void ospSetVec2i(OSPObject, const char *id, int x, int y);
  OSPRAY_INTERFACE void ospSetVec2iv(OSPObject, const char *id, const int *xy);

  OSPRAY_INTERFACE void ospSetVec3f(OSPObject, const char *id, float x, float y, float z);
  OSPRAY_INTERFACE void ospSetVec3fv(OSPObject, const char *id, const float *xyz);
  OSPRAY_INTERFACE void ospSetVec3i(OSPObject, const char *id, int x, int y, int z);
  OSPRAY_INTERFACE void ospSetVec3iv(OSPObject, const char *id, const int *xyz);

  OSPRAY_INTERFACE void ospSetVec4f(OSPObject, const char *id, float x, float y, float z, float w);
  OSPRAY_INTERFACE void ospSetVec4fv(OSPObject, const char *id, const float *xyzw);
  OSPRAY_INTERFACE void ospSetVec4i(OSPObject, const char *id, int x, int y, int z, int w);
  OSPRAY_INTERFACE void ospSetVec4iv(OSPObject, const char *id, const int *xyzw);

  OSPRAY_INTERFACE void ospSetBox1f(OSPObject, const char *id, float lower_x, float upper_x);
  OSPRAY_INTERFACE void ospSetBox1fv(OSPObject, const char *id, const float *lower_x_upper_x);
  OSPRAY_INTERFACE void ospSetBox1i(OSPObject, const char *id, int lower_x, int upper_x);
  OSPRAY_INTERFACE void ospSetBox1iv(OSPObject, const char *id, const int *lower_x_upper_x);

  OSPRAY_INTERFACE void ospSetBox2f(OSPObject, const char *id, float lower_x, float lower_y, float upper_x, float upper_y);
  OSPRAY_INTERFACE void ospSetBox2fv(OSPObject, const char *id, const float *lower_xy_upper_xy);
  OSPRAY_INTERFACE void ospSetBox2i(OSPObject, const char *id, int lower_x, int lower_y, int upper_x, int upper_y);
  OSPRAY_INTERFACE void ospSetBox2iv(OSPObject, const char *id, const int *lower_xy_upper_xy);

  OSPRAY_INTERFACE void ospSetBox3f(OSPObject, const char *id, float lower_x, float lower_y, float lower_z, float upper_x, float upper_y, float upper_z);
  OSPRAY_INTERFACE void ospSetBox3fv(OSPObject, const char *id, const float *lower_xyz_upper_xyz);
  OSPRAY_INTERFACE void ospSetBox3i(OSPObject, const char *id, int lower_x, int lower_y, int lower_z, int upper_x, int upper_y, int upper_z);
  OSPRAY_INTERFACE void ospSetBox3iv(OSPObject, const char *id, const int *lower_xyz_upper_xyz);

  OSPRAY_INTERFACE void ospSetBox4f(OSPObject, const char *id, float lower_x, float lower_y, float lower_z, float lower_w, float upper_x, float upper_y, float upper_z, float upper_w);
  OSPRAY_INTERFACE void ospSetBox4fv(OSPObject, const char *id, const float *lower_xyzw_upper_xyzw);
  OSPRAY_INTERFACE void ospSetBox4i(OSPObject, const char *id, int lower_x, int lower_y, int lower_z, int lower_w, int upper_x, int upper_y, int upper_z, int upper_w);
  OSPRAY_INTERFACE void ospSetBox4iv(OSPObject, const char *id, const int *lower_xyzw_upper_xyzw);

  OSPRAY_INTERFACE void ospSetLinear3fv(OSPObject, const char *id, const float *v);
  OSPRAY_INTERFACE void ospSetAffine3fv(OSPObject, const char *id, const float *v);

  OSPRAY_INTERFACE void ospSetVoidPtr(OSPObject, const char *id, void *v);

  // Object + Parameter Lifetime Management ///////////////////////////////////

  OSPRAY_INTERFACE void ospRemoveParam(OSPObject, const char *id);
  OSPRAY_INTERFACE void ospCommit(OSPObject);
  OSPRAY_INTERFACE void ospRelease(OSPObject);
  OSPRAY_INTERFACE void ospRetain(OSPObject);

  // FrameBuffer Manipulation /////////////////////////////////////////////////

  OSPRAY_INTERFACE OSPFrameBuffer ospNewFrameBuffer(int size_x,
                                                    int size_y,
                                                    OSPFrameBufferFormat format OSP_DEFAULT_VAL(OSP_FB_SRGBA),
                                                    uint32_t frameBufferChannels OSP_DEFAULT_VAL(OSP_FB_COLOR));

  //! create a new pixel op of given type
  /*! return 'NULL' if that type is not known */
  OSPRAY_INTERFACE OSPImageOperation ospNewImageOperation(const char *type);

  /*! \brief map app-side content of a framebuffer (see \ref frame_buffer_handling) */
  OSPRAY_INTERFACE const void *ospMapFrameBuffer(OSPFrameBuffer,
                                                 OSPFrameBufferChannel OSP_DEFAULT_VAL(OSP_FB_COLOR));

  // Unmap a previously mapped frame buffer
  OSPRAY_INTERFACE void ospUnmapFrameBuffer(const void *mapped, OSPFrameBuffer);

  // Get variance from last rendered frame
  OSPRAY_INTERFACE float ospGetVariance(OSPFrameBuffer);

  // Reset frame buffer accumulation for next render frame call
  OSPRAY_INTERFACE void ospResetAccumulation(OSPFrameBuffer);

  // Frame Rendering //////////////////////////////////////////////////////////

  OSPRAY_INTERFACE OSPRenderer ospNewRenderer(const char *type);

  // Render a frame
  OSPRAY_INTERFACE float ospRenderFrame(OSPFrameBuffer, OSPRenderer, OSPCamera, OSPWorld);
  OSPRAY_INTERFACE OSPFuture ospRenderFrameAsync(OSPFrameBuffer, OSPRenderer, OSPCamera, OSPWorld);

  // Ask if all events tracked by an OSPFuture handle have been completed
  OSPRAY_INTERFACE int ospIsReady(OSPFuture, OSPSyncEvent OSP_DEFAULT_VAL(OSP_TASK_FINISHED));

  // Wait on a specific event
  OSPRAY_INTERFACE void ospWait(OSPFuture, OSPSyncEvent OSP_DEFAULT_VAL(OSP_TASK_FINISHED));

  // Cancel the given task (may block calling thread)
  OSPRAY_INTERFACE void ospCancel(OSPFuture);

  // Get the completion state of the given task [0.f-1.f]
  OSPRAY_INTERFACE float ospGetProgress(OSPFuture);

  typedef struct {
    int hasHit;
    float worldPosition[3];
    OSPInstance instance;
    OSPGeometricModel model;
    uint32_t primID;
  } OSPPickResult;

  OSPRAY_INTERFACE void ospPick(OSPPickResult *result,
                                OSPFrameBuffer,
                                OSPRenderer,
                                OSPCamera,
                                OSPWorld,
                                float screenPos_x,
                                float screenPos_y);

#ifdef __cplusplus
} // extern "C"
#endif

#include "detail/ospray_util.h"
