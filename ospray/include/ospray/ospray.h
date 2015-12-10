// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
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

// \file ospray/ospray.h Defines the external OSPRay API */

/*! \defgroup ospray_api OSPRay Core API

  \ingroup ospray

  \brief Defines the public API for the OSPRay core 

  \{
 */

#pragma once

#ifndef NULL
# define NULL nullptr
#endif

#include <vector>

// -------------------------------------------------------
// include common components 
// -------------------------------------------------------
#include <sys/types.h>
#include <stdint.h>

#include "ospray/common/OSPDataType.h"

#ifdef _WIN32
#  ifdef ospray_EXPORTS
#    define OSPRAY_INTERFACE __declspec(dllexport)
#  else
#    define OSPRAY_INTERFACE __declspec(dllimport)
#  endif
#else
#  define OSPRAY_INTERFACE
#endif

#ifdef __GNUC__
  #define OSP_DEPRECATED __attribute__((deprecated))
#elif defined(_MSC_VER)
  #define OSP_DEPRECATED __declspec(deprecated)
#else
  #define OSP_DEPRECATED
#endif

/*! namespace for classes in the public core API */
namespace osp {

  struct vec2f { float x, y; };
  struct vec2i { int x, y; };
  struct vec3f { float x, y, z; };
  struct vec3fa { float x, y, z, a; };
  struct vec3i { int x, y, z; };
  struct vec4f { float x, y, z, w; };
  // typedef embree::Vec2f  vec2f;
  // typedef embree::Vec2i  vec2i;
  // typedef embree::Vec3f  vec3f;
  // typedef embree::Vec3i  vec3i;
  // typedef embree::Vec3fa vec3fa;
  // typedef embree::Vec4f  vec4f;
  struct box2i { vec2i lower, upper; };
  struct box3f { vec2f lower, upper; };
  struct linear3f { vec3f vx,vy,vz; };
  struct affine3f { linear3f l; vec3f p; };
  // typedef embree::BBox<embree::Vec2i> box2i;
  // typedef embree::BBox3f box3f;
  // typedef embree::AffineSpace3f affine3f;

  typedef uint64_t uint64;

  struct ManagedObject    { uint64 ID; virtual ~ManagedObject() {} };
  struct FrameBuffer      : public ManagedObject {};
  struct Renderer         : public ManagedObject {};
  struct Camera           : public ManagedObject {};
  struct Model            : public ManagedObject {};
  struct Data             : public ManagedObject {};
  struct Geometry         : public ManagedObject {};
  struct Material         : public ManagedObject {};
  struct Volume           : public ManagedObject {};
  struct TransferFunction : public ManagedObject {};
  struct Texture2D        : public ManagedObject {};
  struct Light            : public ManagedObject {};
  struct PixelOp          : public ManagedObject {};
  struct TriangleMesh     : public Geometry {};

} // ::osp

typedef enum {
  OSP_FB_COLOR=(1<<0),
  OSP_FB_DEPTH=(1<<1),
  OSP_FB_ACCUM=(1<<2),
//  OSP_FB_ALPHA=(1<<3) // not used anywhere; use OSP_FB_COLOR with a frame buffer format containing alpha in 4th channel
} OSPFrameBufferChannel;

/*! OSPRay constants for Frame Buffer creation ('and' ed together) */
typedef enum {
  OSP_RGBA_NONE,
  OSP_RGBA_I8,  /*!< one dword per pixel: rgb+alpha, each on byte */
  OSP_RGB_I8,   /*!< three 8-bit unsigned chars per pixel XXX unsupported! */
  OSP_RGBA_F32, /*!< one float4 per pixel: rgb+alpha, each one float */
} OSPFrameBufferFormat;

//! constants for switching the OSPRay MPI Scope between 'per rank' and 'all ranks'
/*! \see ospdApiMode */
typedef enum {

  //! \brief all ospNew(), ospSet(), etc calls affect only the current rank 
  /*! \detailed in this mode, all ospXyz() calls made on a given rank
    will ONLY affect state ont hat rank. This allows for configuring a
    (globally known) object differnetly on each different rank (also
    see OSP_MPI_SCOPE_GLOBAL) */
  OSPD_MODE_INDEPENDENT,
  OSPD_RANK=OSPD_MODE_INDEPENDENT /*!< alias for OSP_MODE_INDEPENDENT, reads better in code */,

  //! \brief all ospNew(), ospSet() calls affect all ranks 
  /*! \detailed In this mode, ONLY rank 0 should call ospXyz()
      functions, but all objects defined through those functions---and
      all parameters set through those---will apply equally to all
      ranks. E.g., a OSPVolume vol = ospNewVolume(...) would create a
      volume object handle that exists on (and therefore, is valid on)
      all ranks. The (distributed) app may then switch to 'current
      rank only' mode, and may assign different data or parameters on
      each rank (typically, in order to have different parts of the
      volume on different nodes), but the object itself is globally
      known */
  OSPD_MODE_MASTERED,
  OSPD_MASTER=OSPD_MODE_MASTERED /*!< alias for OSP_MODE_MASTERED, reads better in code */,

  //! \brief all ospNew(), ospSet() are called collaboratively by all ranks 
  /*! \detailed In this mode, ALL ranks must call (the same!) api
      function, the result is collaborative across all nodes in the
      sense that any object being created gets created across all
      nodes, and ALL ranks get a valid handle returned */
  OSPD_MODE_COLLABORATIVE,
  OSPD_ALL=OSPD_MODE_COLLABORATIVE /*!< alias for OSP_MODE_COLLABORATIVE, reads better in code */
} OSPDApiMode;

// /*! flags that can be passed to OSPNewGeometry; can be OR'ed together */
// typedef enum {
//   /*! experimental: currently used to specify that the app ranks -
//       together - hold a logical piece of geometry, with the back-end
//       ospray workers then fetching that on demand..... */
//   OSP_DISTRIBUTED_GEOMETRY = (1<<0),
// } OSPGeometryCreationFlags;

/*! flags that can be passed to OSPNewData; can be OR'ed together */
typedef enum {
  OSP_DATA_SHARED_BUFFER = (1<<0),
} OSPDataCreationFlags;

/*! flags that can be passed to ospNewTexture2D(); can be OR'ed together */
typedef enum {
  OSP_TEXTURE_SHARED_BUFFER = (1<<0),
  OSP_TEXTURE_FILTER_NEAREST = (1<<1) /*!< use nearest-neighbor interpolation rather than the default bilinear interpolation */
} OSPTextureCreationFlags;

typedef enum {
  OSP_OK=0, /*! no error; any value other than zero means 'some kind of error' */
  OSP_GENERAL_ERROR /*! unspecified error */
} OSPResult;

typedef osp::FrameBuffer       *OSPFrameBuffer;
typedef osp::Renderer          *OSPRenderer;
typedef osp::Camera            *OSPCamera;
typedef osp::Model             *OSPModel;
typedef osp::Data              *OSPData;
typedef osp::Geometry          *OSPGeometry;
typedef osp::Material          *OSPMaterial;
typedef osp::Light             *OSPLight;
typedef osp::Volume            *OSPVolume;
typedef osp::TransferFunction  *OSPTransferFunction;
typedef osp::Texture2D         *OSPTexture2D;
typedef osp::TriangleMesh      *OSPTriangleMesh;
typedef osp::ManagedObject     *OSPObject;
typedef osp::PixelOp           *OSPPixelOp;

/*! an error type. '0' means 'no error' */
typedef int32_t error_t;

extern "C" {
  //! initialize the ospray engine (for single-node user application) 
  OSPRAY_INTERFACE void ospInit(int *ac, const char **av);

  typedef enum { 
    OSPD_Z_COMPOSITE
  } OSPDRenderMode;

#ifdef OSPRAY_MPI_DISTRIBUTED
  //! \brief allows for switching the MPI mode btween collaborative, mastered, and independent
  OSPRAY_INTERFACE 
  void ospdApiMode(OSPDApiMode mode);

  //! the 'lid to the pot' of ospdMpiInit(). 
  /*! does both an osp shutdown and an mpi shutdown for the mpi group
      created with ospdMpiInit */
  OSPRAY_INTERFACE 
  void ospdMpiInit(int *ac, char ***av, OSPDRenderMode renderMode=OSPD_Z_COMPOSITE);

  /*! the 'lid to the pot' of ospdMpiInit(). shuts down both ospray
      *and* the MPI layer created with ospdMpiInit */
  OSPRAY_INTERFACE 
  void ospdMpiShutdown();
#endif

  //! load plugin 'name' from shard lib libospray_module_<name>.so
  /*! returns 0 if the module could be loaded, else it returns an error code > 0 */
  OSPRAY_INTERFACE error_t ospLoadModule(const char *pluginName);

  //! use renderer to render a frame. 
  /*! What input to tuse for rendering the frame is encoded in the
      renderer's parameters, typically in "world". */
  OSPRAY_INTERFACE void ospRenderFrame(OSPFrameBuffer fb, 
                                       OSPRenderer renderer, 
                                       const uint32_t fbChannelFlags=OSP_FB_COLOR);

  //! create a new renderer of given type 
  /*! return 'NULL' if that type is not known */
  OSPRAY_INTERFACE OSPRenderer ospNewRenderer(const char *type);
  
  //! create a new pixel op of given type 
  /*! return 'NULL' if that type is not known */
  OSPRAY_INTERFACE OSPPixelOp ospNewPixelOp(const char *type);
  
  //! set a frame buffer's pixel op */
  OSPRAY_INTERFACE void ospSetPixelOp(OSPFrameBuffer fb, OSPPixelOp op);

  //! create a new geometry of given type 
  /*! return 'NULL' if that type is not known */
  OSPRAY_INTERFACE OSPGeometry ospNewGeometry(const char *type);

  //! let given renderer create a new material of given type
  OSPRAY_INTERFACE OSPMaterial ospNewMaterial(OSPRenderer renderer, const char *type);

  //! let given renderer create a new light of given type
  OSPRAY_INTERFACE OSPLight ospNewLight(OSPRenderer renderer, const char *type);
  
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
  OSPRAY_INTERFACE void ospRelease(OSPObject obj);
  
  //! assign given material to given geometry
  OSPRAY_INTERFACE void ospSetMaterial(OSPGeometry geometry, OSPMaterial material);

  //! \brief create a new camera of given type 
  /*! \detailed The default camera type supported in all ospray
    versions is "perspective" (\ref perspective_camera).  For a list
    of supported camera type in this version of ospray, see \ref
    ospray_supported_cameras
    
    \returns 'NULL' if that type is not known, else a handle to the created camera
  */
  OSPRAY_INTERFACE OSPCamera ospNewCamera(const char *type);

  //! \brief create a new volume of given type 
  /*! \detailed return 'NULL' if that type is not known */
  OSPRAY_INTERFACE OSPVolume ospNewVolume(const char *type);

  //! add a volume to an existing model
  OSPRAY_INTERFACE void ospAddVolume(OSPModel model, OSPVolume volume);

  //! \brief create a new transfer function of given type
  /*! \detailed return 'NULL' if that type is not known */
  OSPRAY_INTERFACE OSPTransferFunction ospNewTransferFunction(const char * type);
  
  //! \brief create a new Texture2D with the given parameters
  /*! \detailed return 'NULL' if the texture could not be created with the given parameters */
  OSPRAY_INTERFACE OSPTexture2D ospNewTexture2D(int width, int height, OSPDataType type, void *data = NULL, int flags = 0);

  //! \brief lears the specified channel(s) of the frame buffer
  /*! \detailed clear the specified channel(s) of the frame buffer specified in 'whichChannels'

    if whichChannel&OSP_FB_COLOR!=0, clear the color buffer to '0,0,0,0'
    if whichChannel&OSP_FB_DEPTH!=0, clear the depth buffer to +inf
    if whichChannel&OSP_FB_ACCUM!=0, clear the accum buffer to 0,0,0,0, and reset accumID
  */
  OSPRAY_INTERFACE void ospFrameBufferClear(OSPFrameBuffer fb, const uint32_t whichChannel);

  // -------------------------------------------------------
  /*! \defgroup ospray_data Data Buffer Handling 

    \ingroup ospray_api

    \{ 
  */
  /*! create a new data buffer, with optional init data and control flags 

    Valid flags that can be or'ed together into the flags value:
    - OSP_DATA_SHARED_BUFFER: indicates that the buffer can be shared with the app.
      In this case the calling program guarantees that the 'init' pointer will remain
      valid for the duration that this data array is being used.
   */
  OSPRAY_INTERFACE OSPData ospNewData(size_t numItems, OSPDataType format, const void *init=NULL, int flags=0);

  /*! \} */


  // -------------------------------------------------------
  /*! \defgroup ospray_framebuffer Frame Buffer Manipulation 

      \ingroup ospray_api 
      
      \{ 
  */

  /*! \brief create a new framebuffer (actual format is internal to ospray) 

    Creates a new frame buffer of given size, externalFormat, and channel type(s).

    \param externalFormat describes the format the color buffer has
    *on the host*, and the format that 'ospMapFrameBuffer' will
    eventually return. Valid values are OSP_FB_RBGA_FLOAT,
    OSP_FB_RBGA_I8, OSP_FB_RGB_I8, and OSP_FB_NONE (note that
    OSP_FB_NONE is a perfectly reasonably choice for a framebuffer
    that will be used only internally, see notes below).
    The origin of the screen coordinate system is the lower left
    corner (as in OpenGL).

    \param channelFlags specifies which channels the frame buffer has,
    and is or'ed together from the values OSP_FB_COLOR,
    OSP_FB_DEPTH, and/or OSP_FB_ACCUM. If a certain buffer
    value is _not_ specified, the given buffer will not be present
    (see notes below).
    
    \param size size (in pixels) of frame buffer.

    Note that ospray makes a very clear distinction between the
    _external_ format of the frame buffer, and the internal one(s):
    The external format is the format the user specifies in the
    'externalFormat' parameter; it specifies what format ospray will
    _return_ the frame buffer to the application (up a
    ospMapFrameBuffer() call): no matter what ospray uses internally,
    it will simply return a 2D array of pixels of that format, with
    possibly all kinds of reformatting, compression/decompression,
    etc, going on in-between the generation of the _internal_ frame
    buffer and the mapping of the externally visible one.

    In particular, OSP_FB_NONE is a perfectly valid pixel format for a
    frame buffer that an application will never map. For example, a
    application using multiple render passes (e.g., 'pathtrace' and
    'tone map') may well generate a intermediate frame buffer to store
    the result of the 'pathtrace' stage, but will only ever display
    the output of the tone mapper. In this case, when using a pixel
    format of OSP_FB_NONE the pixels from the path tracing stage will
    never ever be transferred to the application.
   */
  OSPRAY_INTERFACE OSPFrameBuffer ospNewFrameBuffer(const osp::vec2i &size, 
                                    const OSPFrameBufferFormat externalFormat=OSP_RGBA_I8,
                                    const int channelFlags=OSP_FB_COLOR);

  /*! \brief free a framebuffer 

    due to refcounting the frame buffer may not immeidately be deleted
    at this time */
  OSPRAY_INTERFACE void ospFreeFrameBuffer(OSPFrameBuffer fb);

  /*! \brief map app-side content of a framebuffer (see \ref frame_buffer_handling) */
  OSPRAY_INTERFACE const void *ospMapFrameBuffer(OSPFrameBuffer fb, 
                                                 OSPFrameBufferChannel=OSP_FB_COLOR);

  /*! \brief unmap a previously mapped frame buffer (see \ref frame_buffer_handling) */
  OSPRAY_INTERFACE void ospUnmapFrameBuffer(const void *mapped, OSPFrameBuffer fb);

  /*! \} */


  // -------------------------------------------------------
  /*! \defgroup ospray_params Object parameters.

    \ingroup ospray_api

    @{ 
  */
  /*! add a c-string (zero-terminated char *) parameter to another object */
  OSPRAY_INTERFACE void ospSetString(OSPObject _object, const char *id, const char *s);

  /*! add a object-typed parameter to another object 
    
   \warning this call has been superseded by ospSetObject, and will eventually get removed */
  OSP_DEPRECATED OSPRAY_INTERFACE void ospSetParam(OSPObject _object, const char *id, OSPObject object);

  /*! add a object-typed parameter to another object */
  OSPRAY_INTERFACE void ospSetObject(OSPObject _object, const char *id, OSPObject object);

  /*! add a data array to another object */
  OSPRAY_INTERFACE void ospSetData(OSPObject _object, const char *id, OSPData data);

  /*! add 1-float parameter to given object */
  OSPRAY_INTERFACE void ospSetf(OSPObject _object, const char *id, float x);

  /*! add 1-float parameter to given object */
  OSPRAY_INTERFACE void ospSet1f(OSPObject _object, const char *id, float x);

  /*! add 1-int parameter to given object */
  OSPRAY_INTERFACE void ospSet1i(OSPObject _object, const char *id, int32_t x);

  /*! add a 2-float parameter to a given object */
  OSPRAY_INTERFACE void ospSet2f(OSPObject _object, const char *id, float x, float y);

  /*! add 2-float parameter to given object */
  OSPRAY_INTERFACE void ospSet2fv(OSPObject _object, const char *id, const float *xy);

  /*! add a 2-int parameter to a given object */
  OSPRAY_INTERFACE void ospSet2i(OSPObject _object, const char *id, int x, int y);

  /*! add 2-int parameter to given object */
  OSPRAY_INTERFACE void ospSet2iv(OSPObject _object, const char *id, const int *xy);

  /*! add 3-float parameter to given object */
  OSPRAY_INTERFACE void ospSet3f(OSPObject _object, const char *id, float x, float y, float z);

  /*! add 3-float parameter to given object */
  OSPRAY_INTERFACE void ospSet3fv(OSPObject _object, const char *id, const float *xyz);

  /*! add 4-float parameter to given object */
  OSPRAY_INTERFACE void ospSet4f(OSPObject _object, const char *id, float x, float y, float z, float w);

  /*! add 4-float parameter to given object */
  OSPRAY_INTERFACE void ospSet4fv(OSPObject _object, const char *id, const float *xyzw);

  /*! add 3-int parameter to given object */
  OSPRAY_INTERFACE void ospSet3i(OSPObject _object, const char *id, int x, int y, int z);

  /*! add 3-int parameter to given object */
  void ospSet3iv(OSPObject _object, const char *id, const int *xyz);


  // \brief Set a given region of the volume to a given set of voxels
  /*! \detailed Given a block of voxels (of dimensions 'blockDim',
      located at the memory region pointed to by 'source', copy the
      given voxels into volume, at the region of addresses
      [regionCoords...regionCoord+regionSize].
  */
  OSPRAY_INTERFACE int ospSetRegion(/*! the object we're writing this block of pixels into */
                                    OSPVolume object, 
                                    /* points to the first voxel to be copies. The
                                       voxels at 'soruce' MUST have dimensions
                                       'regionSize', must be organized in 3D-array
                                       order, and must have the same voxel type as the
                                       volume.*/
                                    void *source, 
                                    /*! coordinates of the lower, left, front corner of
                                      the target region.*/
                                    const osp::vec3i &regionCoords, 
                                    /*! size of the region that we're writing to; MUST
                                      be the same as the dimensions of source[][][] */
                                    const osp::vec3i &regionSize);

  /*! add 2-float parameter to given object */
  OSPRAY_INTERFACE void ospSetVec2f(OSPObject _object, const char *id, const osp::vec2f &v);

  /*! add 2-int parameter to given object */
  OSPRAY_INTERFACE void ospSetVec2i(OSPObject _object, const char *id, const osp::vec2i &v);

  /*! add 3-float parameter to given object */
  OSPRAY_INTERFACE void ospSetVec3f(OSPObject _object, const char *id, const osp::vec3f &v);

  /*! add 4-float parameter to given object */
  OSPRAY_INTERFACE void ospSetVec4f(OSPObject _object, const char *id, const osp::vec4f &v);

  /*! add 3-int parameter to given object */
  OSPRAY_INTERFACE void ospSetVec3i(OSPObject _object, const char *id, const osp::vec3i &v);

  /*! add untyped void pointer to object - this will *ONLY* work in local rendering!  */
  OSPRAY_INTERFACE void ospSetVoidPtr(OSPObject _object, const char *id, void *v);

  /*! \brief Object and parameter introspection.                            */
  /*!                                                                       */
  /*! These functions are used to retrieve the type or handle of an object, */
  /*! or the name, type, or value of a parameter associated with an object. */
  /*! These functions can be expensive in cases where the host and compute  */
  /*! device are separated by a PCI bus or interconnect.                    */
  /*!                                                                       */
  /*! If the type of the requested parameter does not match that indicated  */
  /*! by the function name, or the named parameter does not exist, then the */
  /*! functions return '0'.                                                 */

  /*! \brief Get the handle of the named data array associated with an object. */
  OSPRAY_INTERFACE int ospGetData(OSPObject object, const char *name, OSPData *value);

  /*! \brief Get a copy of the data in an array (the application is responsible for freeing this pointer). */
  OSPRAY_INTERFACE int ospGetDataValues(OSPData object, void **pointer, size_t *count, OSPDataType *type);

  /*! \brief Get the named scalar floating point value associated with an object. */
  OSPRAY_INTERFACE int ospGetf(OSPObject object, const char *name, float *value);

  /*! \brief Get the named scalar integer associated with an object. */
  OSPRAY_INTERFACE int ospGeti(OSPObject object, const char *name, int *value);

  /*! \brief Get the material associated with a geometry object. */
  OSPRAY_INTERFACE int ospGetMaterial(OSPGeometry geometry, OSPMaterial *value);

  /*! \brief Get the named object associated with an object. */
  OSPRAY_INTERFACE int ospGetObject(OSPObject object, const char *name, OSPObject *value);

  /*! \brief Retrieve a NULL-terminated list of the parameter names associated with an object. */
  OSPRAY_INTERFACE int ospGetParameters(OSPObject object, char ***value);

  /*! \brief Get a pointer to a copy of the named character string associated with an object. */
  OSPRAY_INTERFACE int ospGetString(OSPObject object, const char *name, char **value);

  /*! \brief Get the type of the named parameter or the given object (if 'name' is NULL). */
  OSPRAY_INTERFACE int ospGetType(OSPObject object, const char *name, OSPDataType *value);

  /*! \brief Get the named 2-vector floating point value associated with an object. */
  OSPRAY_INTERFACE int ospGetVec2f(OSPObject object, const char *name, osp::vec2f *value);

  /*! \brief Get the named 3-vector floating point value associated with an object. */
  OSPRAY_INTERFACE int ospGetVec3f(OSPObject object, const char *name, osp::vec3f *value);

  /*! \brief Get the named 3-vector integer value associated with an object. */
  OSPRAY_INTERFACE int ospGetVec3i(OSPObject object, const char *name, osp::vec3i *value);

  /*! @} end of ospray_params */

  // -------------------------------------------------------
  /*! \defgroup ospray_geometry Geometry Handling 
    \ingroup ospray_api
    
    \{ 
  */

  /*! \brief create a new triangle mesh. 

    the mesh doesn't do anything 'til it is added to a model. to set
    the model's vertices etc, set the respective data arrays for
    "position", "index", "normal", "texcoord", "color", etc. Data
    format for vertices and normals in vec3fa, and vec4i for index
    (fourth component is the material ID). */
  //! \warning deprecated: use ospNewGeometry("triangles") instead
  OSP_DEPRECATED OSPRAY_INTERFACE OSPTriangleMesh ospNewTriangleMesh();

  /*! add an already created geometry to a model */
  OSPRAY_INTERFACE void ospAddGeometry(OSPModel model, OSPGeometry mesh);
  /*! \} end of ospray_trianglemesh */

  /*! \brief remove an existing geometry from a model */
  OSPRAY_INTERFACE void ospRemoveGeometry(OSPModel model, OSPGeometry mesh);


  /*! \brief create a new instance geometry that instantiates another
    model.  the resulting geometry still has to be added to another
    model via ospAddGeometry */
  OSPRAY_INTERFACE OSPGeometry ospNewInstance(OSPModel modelToInstantiate,
                                              const osp::affine3f &xfm);

  // -------------------------------------------------------
  /*! \defgroup ospray_model OSPRay Model Handling 
    \ingroup ospray_api

    models are the equivalent of 'scenes' in embree (ie,
    they consist of one or more pieces of content that share a
    single logical acceleration structure); however, models can
    contain more than just embree triangle meshes - they can also
    contain cameras, materials, volume data, etc, as well as
    references to (and instances of) other models.
    
    \{ 
  */

  /*! \brief create a new ospray model.  */
  OSPRAY_INTERFACE OSPModel ospNewModel();

  /*! \} */
  
  /*! \brief commit changes to an object */
  OSPRAY_INTERFACE void ospCommit(OSPObject object);

  /*! \brief represents the result returned by an ospPick operation */
  extern "C" typedef struct {
    osp::vec3f position; //< the position of the hit point (in world-space)
    bool hit;            //< whether or not a hit actually occured
  } OSPPickResult;

  /*! \brief returns the world-space position of the geometry seen at [0-1] normalized screen-space pixel coordinates (if any) */
  OSPRAY_INTERFACE void ospPick(OSPPickResult *result, OSPRenderer renderer, const osp::vec2f &screenPos);

  extern "C" /*OSP_DEPRECATED*/ typedef struct {
    bool hit;
    float world_x, world_y, world_z;
  } OSPPickData;

  /*! \warning this call has been superseded by ospPick, and will eventually get removed */
  OSP_DEPRECATED OSPRAY_INTERFACE OSPPickData ospUnproject(OSPRenderer renderer, const osp::vec2f &screenPos);

  /*! \brief Samples the given volume at the provided world-space coordinates.

    \param results will be allocated by OSPRay and contain the
    resulting sampled values as floats. The application is
    responsible for freeing this memory. In case of error results
    will be NULL.

    This function is meant to provide a _small_ number of samples
    as needed for data probes, etc. in applications. It is not
    intended for large-scale sampling of volumes.
  */
  OSPRAY_INTERFACE void ospSampleVolume(float **results, OSPVolume volume, const osp::vec3f *worldCoordinates, const size_t &count);

} // extern "C"

/*! \} */
