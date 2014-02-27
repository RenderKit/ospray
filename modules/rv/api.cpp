/*! \file api.cpp Implements the hr module api */

// enforce checking of assertions
#undef NDEBUG

// module
#include "ospray/rv_module.h"
// ospray 
#include "common/ospcommon.h"
#include "ospray/camera/camera.h"
// stl 
#include <vector>
#include <map>
// embree
#include "embree2/rtcore.h"
#include "embree2/rtcore_scene.h"
#include "embree2/rtcore_geometry.h"

extern void *rv_layer;

namespace ospray {
  namespace rv {
    using std::cout;
    using std::endl;

    void *ispc_camera = NULL;

    /*! Type of bounding function. */
    // extern void rvGetBounds(void* ptr,              /*!< pointer to user data */
    //                         size_t item,            /*!< item to calculate bounds for */
    //                         RTCBounds& bounds_o     /*!< returns calculated bounds */);
    
// a    /*! Type of intersect function pointer for single rays. */
//     extern void rvIntersect(void* ptr,           /*!< pointer to user data */
//                             RTCRay& ray,         /*!< ray to intersect */
//                             size_t item          /*!< item to intersect */);

    extern "C" void ispc__rv__createModel(RTCScene embreeScene,
                                          uint32 ID,
                                          uint32 numResistors,
                                          const Resistor *resistor,
                                          const float *attribute);

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

    /*! the embree scene handle */
    RTCScene embreeScene = (RTCScene)-1;
    //! set to true every time the scene has changed
    bool     sceneModified = true;

    struct ResistorModel {
      const size_t          numResistors;
      const Resistor *const resistor;
      const float    *const attribute;
      const id_t            ID;
      ResistorModel(const id_t ID,
                    const size_t   numResistors, 
                    const Resistor resistor[],
                    const float    attribute[])
        : ID(ID),
          numResistors(numResistors),
          resistor(resistor),
          attribute(attribute)
      {}
    };

    std::map<id_t,ResistorModel *> resistorModel;



    /*! Type of bounding function. */
    void rvGetBounds(void* ptr,              /*!< pointer to user data */
                     size_t item,            /*!< item to calculate bounds for */
                     RTCBounds& bounds_o     /*!< returns calculated bounds */)
    {
      const ResistorModel *model = (const ResistorModel *)ptr;
      const Resistor &res = model->resistor[item];
      bounds_o.lower_x = res.coordinate.lower.x;
      bounds_o.lower_y = res.coordinate.lower.y;
      bounds_o.upper_x = res.coordinate.upper.x;
      bounds_o.upper_y = res.coordinate.upper.y;

      bounds_o.lower_z = layer[res.layerID].lower_z;
      bounds_o.upper_z = layer[res.layerID].upper_z;
    }
    
    // /*! Type of intersect function pointer for single rays. */
    //  void rvIntersect(void* ptr,           /*!< pointer to user data */
    //                         RTCRay& ray,         /*!< ray to intersect */
    //                         size_t item          /*!< item to intersect */)
    // {
    //   NOTIMPLEMENTED;
    // }





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
      ::rv_layer = layer;
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
      ispc_camera = ((ospray::Camera*)camera)->getIE();
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

    /*! create a new 'group' of resistors, and add them to the model.
      
      the returned id can be used to enable/disable an entire group.  the
      attributes is an array of numResistors * numAttributes floating
      point values, in array-of-structs layout. Ie, the first value is
      the first attribute for the first resistor, the second value the
      second attribute for the first resistor, etc. 
    */ 
    id_t newResistorModel(const size_t   numResistors, 
                          const Resistor resistor[],
                          const float    attribute[])
    {
      Assert2(numResistors > 0, "empty resistor model!?");
      if (embreeScene == (RTCScene)-1) {
        embreeScene = rtcNewScene(RTC_SCENE_COMPACT|
                                  RTC_SCENE_STATIC,//|RTC_SCENE_HIGH_QUALITY,
#if OSPRAY_SPMD_WIDTH==16
                                  RTC_INTERSECT1|RTC_INTERSECT16
#elif OSPRAY_SPMD_WIDTH==8
                                  RTC_INTERSECT1|RTC_INTERSECT8
#elif OSPRAY_SPMD_WIDTH==4
                                  RTC_INTERSECT1|RTC_INTERSECT4
#else
#  error("invalid OSPRAY_SPMD_WIDTH value")
#endif
                                  );
        cout << "#osp:rv: scene created." << endl;
      }
      id_t ID = rtcNewUserGeometry(embreeScene,numResistors);
      ResistorModel *model
        = new ResistorModel(ID,numResistors,resistor,attribute);
      //resistorModel.push_back(model);
      ispc__rv__createModel(embreeScene,ID,numResistors,resistor,attribute);
      // rtcSetUserData(embreeScene,ID,model);
      // rtcSetBoundsFunction(embreeScene,ID,rvGetBounds);
      //      rtcSetIntersectFunction(embreeScene,ID,rvIntersect);
      // ispc__rv__setIntersectFct(embreeScene,ID);//RTCScene scene, uint32 ID);

      resistorModel[ID] = model;
      sceneModified = true;
      cout << "#osp:rv: resistor model created (" << numResistors << " resistors)" << endl;
      return ID;
    }

    void updateEmbreeGeometry()
    {
      if (embreeScene == (RTCScene)-1) 
        throw std::runtime_error("rendering a frame without having created any resistor models!?");
      cout << "#osp:rv: committed embree scene " << embreeScene << endl;
      rtcCommit(embreeScene);
      sceneModified = false;
    }

    /*! render a frame ... */
    void renderFrame()
    {
      if (sceneModified)
        updateEmbreeGeometry();

      ospRenderFrame(fb,renderer);
    }

  }
}

