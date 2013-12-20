#include "ospray/include/ospray/ospray.h"
#include "localdevice.h"

#if 1
# define LOG(a) if (logLevel > 2) std::cout << "#ospray: " << a << std::endl;
#else
# define LOG(a) /*ignore*/
#endif

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
  extern "C" void ospFreeFrameBuffer(OSPFrameBuffer fb)
  {
    ASSERT_DEVICE();
    Assert(fb != NULL);
    NOTIMPLEMENTED;
  }

  extern "C" OSPFrameBuffer ospNewFrameBuffer(const osp::vec2i &size, 
                                              const OSPFrameBufferMode mode,
                                              const size_t swapChainDepth)
  {
    ASSERT_DEVICE();
    Assert(swapChainDepth > 0 && swapChainDepth < 4);
    return ospray::api::Device::current->frameBufferCreate(size,mode,swapChainDepth);
  }

  extern "C" const void *ospMapFrameBuffer(OSPFrameBuffer fb)
  {
    ASSERT_DEVICE();
    Assert(fb != NULL);
    return ospray::api::Device::current->frameBufferMap(fb);
  }
  
  extern "C" void ospUnmapFrameBuffer(const void *mapped,
                                      OSPFrameBuffer fb)
  {
    ASSERT_DEVICE();
    Assert(fb != NULL && "invalid frame buffer in ospAddGeometry");
    Assert(mapped != NULL && "invalid mapped pointer in ospAddGeometry");
    ospray::api::Device::current->frameBufferUnmap(mapped,fb);
  }

  extern "C" OSPModel ospNewModel()
  {
    ASSERT_DEVICE();
    return ospray::api::Device::current->newModel();
  }

  extern "C" void ospFinalizeModel(OSPModel _model)
  {
    ASSERT_DEVICE();
    return ospray::api::Device::current->finalizeModel(_model);
  }

  extern "C" void ospAddGeometry(OSPModel model, OSPGeometry geometry)
  {
    ASSERT_DEVICE();
    Assert(model != NULL && "invalid model in ospAddGeometry");
    Assert(geometry != NULL && "invalid geometry in ospAddGeometry");
    return ospray::api::Device::current->addGeometry(model,geometry);
  }

  /*! create a newa data buffer, with optional init data and control flags */
  extern "C" OSPTriangleMesh ospNewTriangleMesh()
  {
    ASSERT_DEVICE();
    return ospray::api::Device::current->newTriangleMesh();
  }

  /*! create a new data buffer, with optional init data and control flags */
  extern "C" OSPData ospNewData(size_t nitems, OSPDataType format, void *init, int flags)
  {
    ASSERT_DEVICE();
    return ospray::api::Device::current->newData(nitems,format,init,flags);
  }

  /*! add a data array to another object */
  extern "C" void ospSetData(OSPObject object, const char *bufName, OSPData data)
  {
    ASSERT_DEVICE();
    LOG("ospSetData(...,\"" << bufName << "\",...)");
    Assert(object != NULL && "invalid object in ospAddGeometry");
    Assert(bufName != NULL && "invalid buffer name in ospAddId");
    return ospray::api::Device::current->setData(object,bufName,data);
  }

  /*! \brief create a new renderer of given type */
  /*! \detailed return 'NULL' if that type is not known */
  extern "C" OSPRenderer ospNewRenderer(const char *type)
  {
    ASSERT_DEVICE();
    Assert(type != NULL && "invalid render type identifier in ospAddGeometry");
    LOG("ospNewRenderer(" << type << ")");
    OSPRenderer renderer = ospray::api::Device::current->newRenderer(type);
    if (logLevel > 0)
      std::cerr << "#ospray: could not create renderer '" << type << "'" << std::endl;
    return renderer;
  }


}
