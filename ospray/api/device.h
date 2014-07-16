/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

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
                        const OSPFrameBufferFormat mode,
                        const uint32 channels) = 0;
      
      /*! map frame buffer */
      virtual const void *frameBufferMap(OSPFrameBuffer fb, 
                                         OSPFrameBufferChannel) = 0;

      /*! unmap previously mapped frame buffer */
      virtual void frameBufferUnmap(const void *mapped,
                                    OSPFrameBuffer fb) = 0;

      /*! create a new model */
      virtual OSPModel newModel() = 0;

      /*! load module */
      virtual int loadModule(const char *name) = 0;

      /*! commit the given object's outstanding changes */
      virtual void commit(OSPObject object) = 0;

      /*! add a new geometry to a model */
      virtual void addGeometry(OSPModel _model, OSPGeometry _geometry) = 0;
      /*! remove an existing geometry from a model */
      virtual void removeGeometry(OSPModel _model, OSPGeometry _geometry) = 0;

      /*! create a new data buffer */
      virtual OSPData newData(size_t nitems, OSPDataType format, void *init, int flags) = 0;

      /*! assign (named) string parameter to an object */
      virtual void setString(OSPObject object, const char *bufName, const char *s) = 0;
      /*! assign (named) data item as a parameter to an object */
      virtual void setObject(OSPObject object, const char *bufName, OSPObject obj) = 0;
      /*! assign (named) float parameter to an object */
      virtual void setFloat(OSPObject object, const char *bufName, const float f) = 0;
      /*! assign (named) vec3f parameter to an object */
      virtual void setVec3f(OSPObject object, const char *bufName, const vec3f &v) = 0;
      /*! assign (named) int parameter to an object */
      virtual void setInt(OSPObject object, const char *bufName, const int f) = 0;
      /*! assign (named) vec3i parameter to an object */
      virtual void setVec3i(OSPObject object, const char *bufName, const vec3i &v) = 0;
      /*! add untyped void pointer to object - this will *ONLY* work in local rendering!  */
      virtual void setVoidPtr(OSPObject object, const char *bufName, void *v) = 0;

      /*! create a new triangle mesh geometry */
      virtual OSPTriangleMesh newTriangleMesh() = 0;

      /*! create a new renderer object (out of list of registered renderers) */
      virtual OSPRenderer newRenderer(const char *type) = 0;
      
      /*! create a new geometry object (out of list of registered geometries) */
      virtual OSPGeometry newGeometry(const char *type) = 0;
      
      /*! create a new camera object (out of list of registered cameras) */
      virtual OSPCamera newCamera(const char *type) = 0;
      
      /*! create a new volume object (out of list of registered volumes) */
      virtual OSPVolume newVolume(const char *type) = 0;
      
      /*! create a new volume object (out of list of registered volume types) with data from a file */
      virtual OSPVolume newVolumeFromFile(const char *filename, const char *type) = 0;

      /*! create a new transfer function object (out of list of registered transfer function types) */
      virtual OSPTransferFunction newTransferFunction(const char *type) = 0;

      /*! have given renderer create a new material */
      virtual OSPMaterial newMaterial(OSPRenderer _renderer, const char *type) = 0;

      /*! create a new Texture2D object */
      virtual OSPTexture2D newTexture2D(int width, int height, OSPDataType type, void *data, int flags) = 0;

      /*! have given renderer create a new Light */
      virtual OSPLight newLight(OSPRenderer _renderer, const char *type) = 0;

      /*! clear the specified channel(s) of the frame buffer specified in 'whichChannels'
        
        if whichChannel&OSP_FB_COLOR!=0, clear the color buffer to
        '0,0,0,0'.  

        if whichChannel&OSP_FB_DEPTH!=0, clear the depth buffer to
        +inf.  

        if whichChannel&OSP_FB_ACCUM!=0, clear the accum buffer to 0,0,0,0,
        and reset accumID.
      */
      virtual void frameBufferClear(OSPFrameBuffer _fb,
                                    const uint32 fbChannelFlags) = 0; 

      /*! call a renderer to render a frame buffer */
      virtual void renderFrame(OSPFrameBuffer _sc, 
                               OSPRenderer _renderer, 
                               const uint32 fbChannelFlags) = 0;


  
      //! release (i.e., reduce refcount of) given object
      /*! note that all objects in ospray are refcounted, so one cannot
        explicitly "delete" any object. instead, each object is created
        with a refcount of 1, and this refcount will be
        increased/decreased every time another object refers to this
        object resp releases its hold on it; if the refcount is 0 the
        object will automatically get deleted. For example, you can
        create a new material, assign it to a geometry, and immediately
        after this assignation release its refcount; the material will
        stay 'alive' as long as the given geometry requires it. */
      virtual void release(OSPObject _obj) = 0;

      //! assign given material to given geometry
      virtual void setMaterial(OSPGeometry _geom, OSPMaterial _mat) = 0;

      /*! \brief create a new instance geometry that instantiates another
        model.  the resulting geometry still has to be added to another
        model via ospAddGeometry */
      virtual OSPGeometry newInstance(OSPModel modelToInstantiate,
                                      const osp::affine3f &xfm)
      {
        throw "instances not implemented";
      }

    };
  }
}

