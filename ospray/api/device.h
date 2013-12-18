#include "ospray/common/ospcommon.h"
#include "ospray/include/ospray/ospray.h"
/*! \file device.h Defines the abstract base class for OSPRay
    "devices" that implement the OSPRay API */

namespace ospray {
  /*! encapsulates everything related to the implementing public ospray api */
  namespace api {

    /*! abstract base class of all 'devices' that implement the ospray API */
    struct Device {
      /*! singleton that points to currently active device */
      static Device *current;

      virtual ~Device() {};

      /*! create a new frame buffer/swap chain of given type */
      virtual OSPFrameBuffer 
      frameBufferCreate(const vec2i &size, 
                        const OSPFrameBufferMode mode,
                        const size_t swapChainDepth) = 0;
      
      /*! map frame buffer */
      virtual const void *frameBufferMap(OSPFrameBuffer fb) = 0;

      /*! unmap previously mapped frame buffer */
      virtual void frameBufferUnmap(const void *mapped,
                                    OSPFrameBuffer fb) = 0;

      /*! create a new model */
      virtual OSPModel newModel() = 0;

      /*! finalize a newly specified model */
      virtual void finalizeModel(OSPModel _model) = 0;

      /*! add a new geometry to a model */
      virtual void addGeometry(OSPModel _model, OSPGeometry _geometry) = 0;
    };
  }
}

