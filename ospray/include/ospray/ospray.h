/*! \file ospray/ospray.h Public OSPRay API definition file (core components)

  This file defines the public API for the OSPRay core. Since many of
  OSPRays components are intentionally realized in specific modules
  this file will *only* define the very *core* components, with all
  extensions/modules implemented in the respective modules.
 */

#pragma once

// -------------------------------------------------------
// include embree components 
// -------------------------------------------------------
#include "common/math/vec2.h"
#include "common/math/vec3.h"
#include "common/math/bbox.h"

/*! namespace for classes in the public core API */
namespace osp {
  typedef embree::Vec2i  vec2i;
  typedef embree::Vec3f  vec3f;
  typedef embree::Vec3fa vec3fa;

  struct ManagedObject { unsigned long ID; virtual ~ManagedObject() {} };
  struct FrameBuffer : public ManagedObject {};
  struct Renderer    : public ManagedObject {};
  struct Model       : public ManagedObject {};
}

/* OSPRay constants for Frame Buffer creation ('and' ed together) */
typedef enum {
  OSP_RGBA_I8,  /*!< one dword per pixel: rgb+alpha, each on byte */
  OSP_RGBA_F32, /*!< one float4 per pixel: rgb+alpha, each one float */
  OSP_RGBZ_F32, /*!< one float4 per pixel: rgb+depth, each one float */ 
} OSPFrameBufferMode;

typedef enum {
  OSP_INT, OSP_FLOAT
} OSPDataType;

typedef osp::FrameBuffer *OSPFrameBuffer;
typedef osp::Renderer    *OSPRenderer;
typedef osp::Model       *OSPModel;

/*! \defgroup cplusplus_api Public OSPRay Core API */
/*! @{ */
extern "C" {
  /*! initialize the ospray engine (for single-node user application) */
  void ospInit(int *ac, const char **av);
  /*! use renderer to render given model into specified frame buffer */
  void ospRenderFrame(OSPFrameBuffer fb, OSPRenderer renderer, OSPModel model);

  // -------------------------------------------------------
  /*! \defgroup ospray_framebuffer Frame Buffer Manipulation API */
  /*! @{ */
  /*! create a new framebuffer (actual format is internal to ospray) */
  OSPFrameBuffer ospFrameBufferCreate(const osp::vec2i &size, 
                                      const OSPFrameBufferMode mode=OSP_RGBA_I8,
                                      const size_t swapChainDepth=2);
  /*! destroy a framebuffer */
  void ospFrameBufferDestroy(OSPFrameBuffer fb);
  /*! map app-side content of a framebuffer (see \ref frame_buffer_handling) */
  const void *ospFrameBufferMap(OSPFrameBuffer fb);
  /*! unmap a previously mapped frame buffer (see \ref frame_buffer_handling) */
  void ospFrameBufferUnmap(const void *mapped, OSPFrameBuffer fb);

  /*! (not implemented yet) add a additional dedicated channel to a
    frame buffer. ie, add a (OSPint:"ID0") channel for ID rendering,
    or a "OSPFloat:"depth" depth channel (in addition to the primary
    RGBA_F32 buffer), etc). Note that the channel inherits the
    framebuffer's default compression mode (with the data type
    indicating the best compression mode) */
  void ospFrameBufferChannelAdd(const char *which, OSPDataType data);
  /*! (not implemented yet) map a specific (non-default) frame buffer
      channel by name */
  const void *ospFrameBufferChannelMap(OSPFrameBuffer fb, const char *channel);

  /*! @} end of ospray_framebuffer */

}
/*! @} */
