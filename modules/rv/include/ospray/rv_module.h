/*! file haroon.h \brief Defines the domain-specific API for the Haroon module (\ref haroon_module) */

#pragma once

// include the bulk of the ospray API
#include "ospray/ospray.h"

// add haroon-specific extensions
namespace ospray {
  namespace rv {
    typedef uint32 id_t;

    typedef embree::Vec2i  vec2i;
    typedef embree::Vec3f  vec3f;
    typedef embree::Vec3i  vec3i;
    typedef embree::Vec3fa vec3fa;
    typedef embree::BBox<embree::Vec2i> box2i;
    typedef embree::BBox<embree::Vec2f> box2f;
    typedef embree::BBox<embree::Vec3f> box3f;

    // Render mode
    typedef enum {
      RENDER_BY_LAYER,
      RENDER_BY_NET,
      RENDER_BY_ATTRIBUTE
    } RenderMode;

    // ------------------------------------------------------------------
    // geometry/transistors/layers specific part
    // ------------------------------------------------------------------

    /*! \brief abstraction for a transistor */
    struct Resistor {
      box2f  coordinate;   //!< coordinates, in nm
      uint32 layerID;      //!< layer ID that this resistor is in - provides y coords
      uint32 netID;        //!< net that this resistor is in
    };

    /*! \brief abstraction for a 'net'. 

      \note (iw): I don't really know what nets are for so far, so
      i'll just specify 'define' a net by a color */
    struct Net {
      //! color to be used when in RENDER_BY_NET mode
      vec3f color;
    };
    struct Layer {
      //! color to be used when in RENDER_BY_LAYER mode
      vec3f color;
      float lower_z, upper_z;
    };
    //! color range to be used in RENDER_BY_ATTRIBUTE mode 
    /*! the attribute value of the resistor hit by the ray will be
     used to linearly interpolate between the 'lo' and 'hi'
     values. essentially this is a linear transfer function */
    struct ColorRange {
      vec3f lo_color;
      float lo_value;
      vec3f hi_color;
      float hi_value;
    };

    //! set shade mode to shade by network
    void shadeByNet();
    //! set shade mode to shade by layer
    void shadeByLayer();
    //! set shade mode to shade by attribute
    /* SHADE_BY_ATTRIBUTE mode.  if a ray hits a transistor with given
       attribID, it will first read the associated attribute value for
       this trnasistor, then put that attribute value into the given
       color range for linerat interpolation. the resulting linearly
       interpolated color will then be used for shading */ 
    void shadeByAttribute(id_t attribID, const ColorRange &range);
    
    /*! \brief inititalize the RV module. 

      has to be called *exactly* once, at the beginning 

      \warning Make sure you're calling ospray::rv::initRV, *NOT*
      ospray::init from the parent namespace
     */
    void initRV(int *ac, const char **av);

    // /*! specify the attribute range used when shading in
    //   SHADE_BY_ATTRIBUTE mode.  if a ray hits a transistor with given
    //   attribID, it will first read the associated attribute value for
    //   this trnasistor, then put that attribute value into the given
    //   color range for linerat interpolation. the resulting linearly
    //   interpolated color will then be used for shading */
    // void setAttributeColorRange(id_t attribID, const ColorRange &range);
    
    /*! \brief specify the set of layers we will be using. must be called exactly once.

        We will create a copy of the passed array, so the app is free to free the layer[]
        mem after this call.
    */
    void setLayers(const Layer layer[], const size_t numLayers);

    /*! \brief specify number of attributes stored per resistor. 
      value *must* be specified before rendering the first frame; value
      must be the same for all models used in rendering that frame */
    void setNumAttributesPerResistor(uint32 numAttributesPerResistor);

    /*! \brief specify the set of nets we will be using. must be called exactly once

        We will create a copy of the passed array, so the app is free to free the layer[]
        mem after this call.
    */
    void setNets(const Net net[], const size_t numNets);
    
    /*! create a new 'group' of resistors, and add them to the model.
      
      the returned id can be used to enable/disable an entire group.  the
      attributes is an array of numResistors * numAttributes floating
      point values, in array-of-structs layout. Ie, the first value is
      the first attribute for the first resistor, the second value the
      second attribute for the first resistor, etc. 
    */ 
    id_t newResistorModel(const size_t   numResistors, 
                          const Resistor resistors[],
                          const float    attributes[]);
    
    //! free given group of resistors - not yet implemented
    void freeResistorModel(id_t groupID);
    //! set visibilty for given group of resistors
    void setModelVisibility(const id_t groupID, bool visible);
    //! set visibilty for given layer
    void setLayerVisibility(const id_t layerID, bool visible);
    //! set visibilty for given net
    void setNetVisibility(const id_t netID, bool visible);

    // ------------------------------------------------------------------
    // shading related part
    // ------------------------------------------------------------------

    // ------------------------------------------------------------------
    // camera and frame buffer specific part
    // ------------------------------------------------------------------

    //! set the camera
    void setCamera(const vec3f &pos, const vec3f &at, const vec3f &up, 
                   const float aspect, const float angle);
    
    /*! \brief set frame buffer resolution
      
      this function must be called at least once before rendering a frame */
    void setFrameBufferSize(const vec2i &size);

    /*! \brief map frame buffer 
    
      the returned pointer points to the array of pixels (of size
      specified in last call to ospRVFrameBufferResize). The pixel array
      is only valid after calling this frame, and only until the next
      call to 'unmap'. A frame buffer can be mapped only once, and has
      to be properly unmapped before it can be used to render the next
      frame */
    const uint32 *mapFB();
    //! unmap previously mapped frame buffer
    void          unmapFB();

    /*! render a frame ... */
    void renderFrame();

    // ------------------------------------------------------------------
    // query interface
    // ------------------------------------------------------------------

    /*! struct filled in by ospRVPick */
    struct PickInfo {
      uint32 groupID;
      uint32 transistorID;
    };
    /*! shoot a ray through given pixel coordinates, and fill in the
      query record. returns true if successful, or false in case of 'no
      hit below this pixel' */
    bool pickAtPixel(const uint32 pixel_x, const uint32 pixel_y,
                     PickInfo &pickInfo);
  
    /*! query all objects in given screen region, and return array with
      it.  array is alloced by ospray, and should be free'd by
      application. 'fullyin' may or may not be supported initially */
    PickInfo *queryResistorsInBox(const box2i &region, bool fullyIn);        
  }
}

