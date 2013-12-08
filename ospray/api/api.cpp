#include "ospray/include/ospray/ospray.h"
#include "localdevice.h"

/*! \file api.cpp implements the public ospray api functions by
    routing them to a respective \ref device */
namespace ospray {
#define ASSERT_DEVICE() if (ospray::api::Device::current == NULL)       \
    throw std::runtime_error("OSPRay not yet initialized "              \
                             "(most likely this means you tried to "    \
                             "call an ospray API function before "      \
                             "first calling ospInit())");

  
  extern "C" void ospInit(int *_ac, const char **_av) 
  {
    if (ospray::api::Device::current) 
      throw std::runtime_error("OSPRay error: device already exists "
                               "(did you call ospInit twice?)");

    // we're only supporting local rendering for now - network device
    // etc to come.
    ospray::api::Device::current = new ospray::api::LocalDevice;
  }

  /*! destroy a given frame buffer. 

   due to internal reference counting the framebuffer may or may not be deleted immeidately
  */
  extern "C" void ospFrameBufferDestroy(OSPFrameBuffer fb)
  {
    ASSERT_DEVICE();
    Assert(fb != NULL);
    NOTIMPLEMENTED;
  }

  extern "C" OSPFrameBuffer ospFrameBufferCreate(const osp::vec2i &size, 
                                                 const OSPFrameBufferMode mode,
                                                 const size_t swapChainDepth)
  {
    ASSERT_DEVICE();
    Assert(swapChainDepth > 0 && swapChainDepth < 4);
    return ospray::api::Device::current->frameBufferCreate(size,mode,swapChainDepth);
  }

  extern "C" const void *ospFrameBufferMap(OSPFrameBuffer fb)
  {
    ASSERT_DEVICE();
    Assert(fb != NULL);
    return ospray::api::Device::current->frameBufferMap(fb);
  }
  
  extern "C" void ospFrameBufferUnmap(const void *mapped,
                                      OSPFrameBuffer fb)
  {
    ASSERT_DEVICE();
    Assert(fb != NULL);
    Assert(mapped != NULL);
    ospray::api::Device::current->frameBufferUnmap(mapped,fb);
  }
}
