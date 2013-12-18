#include "device.h"

/*! \file localdevice.h Implements the "local" device for local rendering */

namespace ospray {
  namespace api {
    struct LocalDevice : public Device {

      virtual OSPFrameBuffer frameBufferCreate(const vec2i &size, 
                                               const OSPFrameBufferMode mode,
                                               const size_t swapChainDepth);

      /*! map frame buffer */
      virtual const void *frameBufferMap(OSPFrameBuffer fb);

      /*! unmap previously mapped frame buffer */
      virtual void frameBufferUnmap(const void *mapped,
                                    OSPFrameBuffer fb);

      /*! create a new model */
      virtual OSPModel newModel();

      /*! finalize a newly specified model */
      virtual void finalizeModel(OSPModel _model);

      /*! add a new geometry to a model */
      virtual void addGeometry(OSPModel _model, OSPGeometry _geometry);
    };
  }
}


