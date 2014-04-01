/*! \file haroon.h
  
  \brief Defines the domain-specific API for the Haroon module (\ref haroon_module) 

*/

#pragma once

// include the bulk of the ospray API
#include "ospray/ospray.h"

// add haroon-specific extensions
extern "C" {


  // ------------------------------------------------------------------
  // geometry/transistors/layers specific part
  // ------------------------------------------------------------------

  /*! \brief abstraction for a transistor */
  struct Transistor {
    box2f  coordinates;  //!< transistor coordinates 
    uint32 layerID;      //!< layer ID that this transistor is in - provides y coords
    /*! \brief ID that a user can add to this transistor and query via 'pick'

      This is also the value used to index into the attribute arrays */
    uint32 transistorID; 
  };

  void ospHrNewTransistors(const Transistor *array, const size_t 

  // ------------------------------------------------------------------
  // camera and frame buffer specific part
  // ------------------------------------------------------------------

  //! set the camera
  void ospHrSetCamera(const vec3f &pos, const vec3f &at, const vec3f &up, const float angle);

  /*! \brief set frame buffer resolution
    
    this function must be called at least once before rendering a frame */
  void ospHrFrameBufferResize(const vec2i &size);

  /*! \brief map frame buffer 
    
    the returned pointer points to the array of pixels (of size
    specified in last call to ospHrFrameBufferResize). The pixel array
    is only valid after calling this frame, and only until the next
    call to 'unmap'. A frame buffer can be mapped only once, and has
    to be properly unmapped before it can be used to render the next
    frame */
  const uint32 *ospHrFrameBufferMap();
  //! unmap previously mapped frame buffer
  void          ospHrFarmeBufferUnmap();

}

