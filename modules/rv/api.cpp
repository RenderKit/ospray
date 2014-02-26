/*! \file api.cpp Implements the hr module api */

// enforce checking of assertions
#undef NDEBUG

#include "ospray/rv_module.h"
#include <vector>
#include "common/ospcommon.h"

namespace ospray {
  namespace rv {
    using std::cout;
    using std::endl;

    //! array of layers to be used during rendering
    Layer *layer     = NULL;
    //! num layers to be used during rendering
    size_t numLayers = -1;
    //! whether the given layer is enabled
    bool *layerEnabled;

    //! array of nets to be used during rendering
    Net   *net     = NULL;
    //! num nets to be used during rendering
    size_t numNets = -1;

    //! the renderer we use for rendering
    OSPRenderer    renderer = NULL;
    //! the camera we use for rendering
    OSPCamera      camera = NULL;
    //! the frame buffer we're rendering into
    OSPFrameBuffer fb     = NULL;
    const void    *mappedFB = NULL;

    RenderMode shadeMode = RENDER_BY_LAYER;
    ColorRange shadedAttributeRange;
    int        shadedAttributeID = -1;

    /*! \brief inititalize the RV module. 

      has to be called *exactly* once, at the beginning 

      \warning Make sure you're calling ospray::rv::initRV, *NOT*
      ospray::init from the parent namespace
     */
    void initRV(int *ac, const char **av)
    {
      ospInit(ac,av);
      cout << "#osp:rv: rv module initialized." << endl;

      ospLoadPlugin("rv");
      renderer = ospNewRenderer("rv");
      Assert2(renderer,"could not load renderer");
    }


    /*! \brief specify the set of nets we will be using. must be called exactly once

        We will create a copy of the passed array, so the app is free to free the layer[]
        mem after this call.
    */
    void setNets(const Net appNet[], const size_t appNumNets)
    {
      Assert2(ospray::rv::net == NULL, "Nets specified more than once");
      Assert2(appNet != NULL, "App specified invalid nets");
      Assert2(appNumNets > 0, "App specified invalid nets");

      numNets = appNumNets;
      net     = new Net[numNets];
      for (int i=0;i<numNets;i++)
        net[i] = appNet[i];
      cout << "#osp:rv: nets specified." << endl;
    }

    /*! \brief specify the set of layers we will be using. must be called exactly once.

        We will create a copy of the passed array, so the app is free to free the layer[]
        mem after this call.
    */
    void setLayers(const Layer appLayer[], const size_t appNumLayers)
    {
      Assert2(ospray::rv::layer == NULL, "Layers specified more than once");
      Assert2(appLayer != NULL, "App specified invalid layers");
      Assert2(appNumLayers > 0, "App specified invalid layers");

      numLayers = appNumLayers;
      layer     = new Layer[numLayers];
      layerEnabled = new bool[numLayers];
      for (int i=0;i<numLayers;i++) {
        layer[i] = appLayer[i];
        layerEnabled[i] = 1;
      }
      cout << "#osp:rv: layers specified." << endl;
    }

    //! set visibilty for given layer
    void setLayerVisibility(const id_t layerID, bool visible)
    {
      Assert2(ospray::rv::layer != NULL, "Layers not yet defined");
      Assert2(ospray::rv::layerEnabled != NULL, "Layers not yet defined");
      Assert2(layerID >= 0 && layerID < numLayers, "invalid layer ID");
      layerEnabled[layerID] = visible;
    }

    void resetAccum()
    {
    }

    //! set shade mode to shade by network
    void shadeByNet()
    {
      shadeMode = RENDER_BY_NET;
      resetAccum();
    }
    //! set shade mode to shade by layer
    void shadeByLayer()
    {
      shadeMode = RENDER_BY_LAYER;
      resetAccum();
    }
    //! set shade mode to shade by attribute
    /* SHADE_BY_ATTRIBUTE mode.  if a ray hits a transistor with given
       attribID, it will first read the associated attribute value for
       this trnasistor, then put that attribute value into the given
       color range for linerat interpolation. the resulting linearly
       interpolated color will then be used for shading */ 
    void shadeByAttribute(id_t attribID, const ColorRange &range)
    {
      shadedAttributeRange = range;
      shadedAttributeID    = attribID;
      shadeMode = RENDER_BY_ATTRIBUTE;
      resetAccum();
    }

    /*! \brief set frame buffer resolution
      
      this function must be called at least once before rendering a frame */
    void setFrameBufferSize(const vec2i &size)
    {
      if (fb != NULL)
        ospFreeFrameBuffer(fb);
      fb = ospNewFrameBuffer(size,OSP_RGBA_I8);
      cout << "#osp:rv: frame buffer resized to " << size << endl;
    }
    

    //! set the camera
    void setCamera(const vec3f &pos, const vec3f &at, const vec3f &up, 
                   const float aspect, const float angle)
    {
      if (camera == NULL)
        camera = ospNewCamera("perspective");
      ospSetVec3f(camera,"pos",pos);
      ospSetVec3f(camera,"dir",at-pos);
      ospSetVec3f(camera,"up",up);
      ospSetVec3f(camera,"angle",angle);
      ospSetVec3f(camera,"aspect",aspect);
      ospCommit(camera);
    }
    

    /*! \brief map frame buffer 
    
      the returned pointer points to the array of pixels (of size
      specified in last call to ospRVFrameBufferResize). The pixel array
      is only valid after calling this frame, and only until the next
      call to 'unmap'. A frame buffer can be mapped only once, and has
      to be properly unmapped before it can be used to render the next
      frame */
    const uint32 *mapFB()
    {
      Assert2(fb != NULL,"Frame Buffer not yet initialized!?");
      mappedFB = ospMapFrameBuffer(fb);
      Assert2(mappedFB,"counld not map frame buffer");
      return (uint32 *)mappedFB;
    }

    //! unmap previously mapped frame buffer
    void          unmapFB()
    {
      Assert2(fb != NULL,"Frame Buffer not yet initialized!?");
      ospUnmapFrameBuffer(mappedFB,fb);   
    }

  }
}

