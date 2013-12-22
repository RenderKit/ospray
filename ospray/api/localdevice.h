#include "device.h"

/*! \file localdevice.h Implements the "local" device for local rendering */

namespace ospray {
  namespace api {
    struct LocalDevice : public Device {

      /*! constructor */
      LocalDevice();

      /*! create a new frame buffer */
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

      /*! create a new data buffer */
      virtual OSPData newData(size_t nitems, OSPDataType format, void *init, int flags);

      /*! assign (named) data item as a parameter to an object */
      virtual void setData(OSPObject object, const char *bufName, OSPData data);

      /*! create a new triangle mesh geometry */
      virtual OSPTriangleMesh newTriangleMesh();

      /*! create a new renderer object (out of list of registered renderers) */
      virtual OSPRenderer newRenderer(const char *type);

      /*! call a renderer to render a frame buffer */
      virtual void renderFrame(OSPFrameBuffer fb, 
                               OSPRenderer renderer, 
                               OSPModel model);
    };
  }
}


