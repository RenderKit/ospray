// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

#include "MPICommon.h"
#include "ospray/api/Device.h"
#include "ospray/device/command.h"
#include "CommandStream.h"
#include "ospray/common/Managed.h"

/*! \file mpidevice.h Implements the "mpi" device for mpi rendering */

namespace ospray {
  namespace mpi {

    struct MPIDevice : public api::Device {
      typedef ospray::mpi::CommandStream CommandStream;

      CommandStream cmd;

      typedef ospray::CommandTag CommandTag;

      /*! constructor */
      MPIDevice(// AppMode appMode, OSPMode ospMode,
                int *_ac=NULL, const char **_av=NULL);

      /*! create a new frame buffer */
      OSPFrameBuffer
      frameBufferCreate(const vec2i &size, 
                        const OSPFrameBufferFormat mode,
                        const uint32 channels) override;

      /*! create a new transfer function object (out of list of 
        registered transfer function types) */
      OSPTransferFunction newTransferFunction(const char *type) override;

      /*! have given renderer create a new Light */
      OSPLight newLight(OSPRenderer _renderer, const char *type) override;

      /*! map frame buffer */
      const void *frameBufferMap(OSPFrameBuffer fb,
                                 OSPFrameBufferChannel channel) override;

      /*! unmap previously mapped frame buffer */
      void frameBufferUnmap(const void *mapped, OSPFrameBuffer fb) override;


      /*! set a frame buffer's pixel op object */
      void setPixelOp(OSPFrameBuffer _fb, OSPPixelOp _op) override;
      
      /*! create a new pixelOp object (out of list of registered pixelOps) */
      OSPPixelOp newPixelOp(const char *type) override;
        
      /*! clear the specified channel(s) of the frame buffer specified in 'whichChannels'
        
        if whichChannel&OSP_FB_COLOR!=0, clear the color buffer to
        '0,0,0,0'.  

        if whichChannel&OSP_FB_DEPTH!=0, clear the depth buffer to
        +inf.  

        if whichChannel&OSP_FB_ACCUM!=0, clear the accum buffer to 0,0,0,0,
        and reset accumID.
      */
      void frameBufferClear(OSPFrameBuffer _fb,
                                    const uint32 fbChannelFlags) override;

      /*! create a new model */
      OSPModel newModel() override;

      // /*! finalize a newly specified model */
      // void finalizeModel(OSPModel _model) override;

      /*! commit the given object's outstanding changes */
      void commit(OSPObject object) override;

      /*! add a new geometry to a model */
      void addGeometry(OSPModel _model, OSPGeometry _geometry) override;

      /*! remove an existing geometry from a model */
      void removeGeometry(OSPModel _model, OSPGeometry _geometry) override;

      /*! add a new volume to a model */
      void addVolume(OSPModel _model, OSPVolume _volume) override;

      /*! remove an existing volume from a model */
      void removeVolume(OSPModel _model, OSPVolume _volume) override;

      /*! create a new data buffer */
      OSPData newData(size_t nitems, OSPDataType format,
                      void *init, int flags) override;

      /*! Copy data into the given volume. */
      int setRegion(OSPVolume object, const void *source,
                            const vec3i &index, const vec3i &count) override;

      /*! assign (named) string parameter to an object */
      void setString(OSPObject object,
                     const char *bufName,
                     const char *s) override;

      /*! assign (named) data item as a parameter to an object */
      void setObject(OSPObject target,
                     const char *bufName,
                     OSPObject value) override;

      /*! assign (named) float parameter to an object */
      void setFloat(OSPObject object,
                    const char *bufName,
                    const float f) override;

      /*! assign (named) vec2f parameter to an object */
      void setVec2f(OSPObject object,
                    const char *bufName,
                    const vec2f &v) override;

      /*! assign (named) vec3f parameter to an object */
      void setVec3f(OSPObject object,
                    const char *bufName,
                    const vec3f &v) override;

      /*! assign (named) vec4f parameter to an object */
      void setVec4f(OSPObject object,
                    const char *bufName,
                    const vec4f &v) override;

      /*! assign (named) int parameter to an object */
      void setInt(OSPObject object, const char *bufName, const int f) override;

      /*! assign (named) vec2i parameter to an object */
      void setVec2i(OSPObject object,
                    const char *bufName,
                    const vec2i &v) override;

      /*! assign (named) vec3i parameter to an object */
      void setVec3i(OSPObject object,
                    const char *bufName,
                    const vec3i &v) override;

      /*! add untyped void pointer to object - this will *ONLY* work in local rendering!  */
      void setVoidPtr(OSPObject object, const char *bufName, void *v) override;

      /*! Get the handle of the named data array associated with an object. */
      int getData(OSPObject object, const char *name, OSPData *value) override;

      /*! Get a copy of the data in an array (the application is responsible for freeing this pointer). */
      int getDataValues(OSPData object, void **pointer,
                        size_t *count, OSPDataType *type) override;

      /*! Get the named scalar floating point value associated with an object. */
      int getf(OSPObject object, const char *name, float *value) override;

      /*! Get the named scalar integer associated with an object. */
      int geti(OSPObject object, const char *name, int *value) override;

      /*! Get the material associated with a geometry object. */
      int getMaterial(OSPGeometry geometry, OSPMaterial *value) override;

      /*! Get the named object associated with an object. */
      int getObject(OSPObject object,
                    const char *name,
                    OSPObject *value) override;

      /*! Retrieve a NULL-terminated list of the parameter names associated with an object. */
      int getParameters(OSPObject object, char ***value) override;

      /*! Retrieve the total length of the names (with terminators) of the parameters associated with an object. */
      int getParametersSize(OSPObject object, int *value);

      /*! Get a pointer to a copy of the named character string associated with an object. */
      int getString(OSPObject object,
                    const char *name,
                    char **value) override;

      /*! Get the type of the named parameter or the given object (if 'name' is NULL). */
      int getType(OSPObject object,
                  const char *name,
                  OSPDataType *value) override;

      /*! Get the named 2-vector floating point value associated with an object. */
      int getVec2f(OSPObject object, const char *name, vec2f *value) override;

      /*! Get the named 3-vector floating point value associated with an object. */
      int getVec3f(OSPObject object, const char *name, vec3f *value) override;

      /*! Get the named 4-vector floating point value associated with an object. */
      int getVec4f(OSPObject object, const char *name, vec4f *value) override;

      /*! Get the named 3-vector integer value associated with an object. */
      int getVec3i(OSPObject object, const char *name, vec3i *value) override;

      /*! create a new renderer object (out of list of registered renderers) */
      OSPRenderer newRenderer(const char *type) override;

      /*! create a new geometry object (out of list of registered geometrys) */
      OSPGeometry newGeometry(const char *type) override;

      /*! have given renderer create a new material */
      OSPMaterial newMaterial(OSPRenderer _renderer, const char *type) override;

      /*! create a new camera object (out of list of registered cameras) */
      OSPCamera newCamera(const char *type) override;

      /*! create a new volume object (out of list of registered volumes) */
      OSPVolume newVolume(const char *type) override;

      /*! call a renderer to render a frame buffer */
      void renderFrame(OSPFrameBuffer _sc,
                               OSPRenderer _renderer, 
                               const uint32 fbChannelFlags) override;

      /*! load module */
      int loadModule(const char *name) override;

      //! release (i.e., reduce refcount of) given object
      /*! note that all objects in ospray are refcounted, so one cannot
        explicitly "delete" any object. instead, each object is created
        with a refcount of 1, and this refcount will be
        increased/decreased every time another object refers to this
        object resp releases its hold on it override; if the refcount is 0 the
        object will automatically get deleted. For example, you can
        create a new material, assign it to a geometry, and immediately
        after this assignation release its refcount override; the material will
        stay 'alive' as long as the given geometry requires it. */
      void release(OSPObject _obj) override;

      //! assign given material to given geometry
      void setMaterial(OSPGeometry _geom, OSPMaterial _mat) override;

      /*! create a new Texture2D object */
      OSPTexture2D newTexture2D(const vec2i &size, const OSPTextureFormat,
                                void *data, const uint32 flags) override;

      /*! switch API mode for distriubted API extensions */
      void apiMode(OSPDApiMode mode) override;

      OSPDApiMode currentApiMode;

      /*! sample a volume */
      void sampleVolume(float **results,
                        OSPVolume volume,
                        const vec3f *worldCoordinates,
                        const size_t &count) override;

    };

    // ==================================================================
    // Helper functions
    // ==================================================================

    /*! return a string represenging the given API Mode */
    const char *apiModeName(int mode);

  } // ::ospray::mpi
} // ::ospray


