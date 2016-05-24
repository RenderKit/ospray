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

// \file ospray/ospray.h Defines the external OSPRay API */

/*! \defgroup ospray_api OSPRay Core API

  \ingroup ospray

  \brief Defines the public API for the OSPRay core

  \{
*/

#pragma once

#ifndef NULL
# if __cplusplus >= 201103L
#  define NULL nullptr
# else
#  define NULL 0
# endif
#endif

// -------------------------------------------------------
// include common components
// -------------------------------------------------------
#include <sys/types.h>
#include <stdint.h>

#include "ospray/OSPDataType.h"
#include "ospray/OSPTexture.h"

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

/*! OSPRay format constants for Frame Buffer creation */
typedef enum {
  OSP_FB_NONE,    //!< framebuffer will not be mapped by application
  OSP_FB_RGBA8,   //!< one dword per pixel: rgb+alpha, each one byte
  OSP_FB_SRGBA,   //!< one dword per pixel: rgb (in sRGB space) + alpha, each one byte
  OSP_FB_RGBA32F, //!< one float4 per pixel: rgb+alpha, each one float
/* TODO
  OSP_FB_RGB8,    //!< three 8-bit unsigned chars per pixel
  OSP_FB_RGB32F,  ?
  OSP_FB_SRGB,    //!< three 8-bit unsigned chars (in sRGB space) per pixel
*/
} OSPFrameBufferFormat;

/*! OSPRay channel constants for Frame Buffer (can be OR'ed together) */
typedef enum {
  OSP_FB_COLOR=(1<<0),
  OSP_FB_DEPTH=(1<<1),
  OSP_FB_ACCUM=(1<<2),
  OSP_FB_VARIANCE=(1<<3)
} OSPFrameBufferChannel;

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

/*! flags that can be passed to OSPNewGeometry; can be OR'ed together */
typedef enum {
  /*! experimental: currently used to specify that the app ranks -
      together - hold a logical piece of geometry, with the back-end
      ospray workers then fetching that on demand..... */
  OSP_DISTRIBUTED_GEOMETRY = (1<<0),
} OSPGeometryCreationFlags;

/*! flags that can be passed to OSPNewData; can be OR'ed together */
typedef enum {
  OSP_DATA_SHARED_BUFFER = (1<<0),
} OSPDataCreationFlags;

typedef enum {
  OSPD_Z_COMPOSITE
} OSPDRenderMode;


#ifdef __cplusplus
namespace osp {
  /*! namespace for classes in the public core API */

#ifdef OSPRAY_EXTERNAL_VECTOR_TYPES
  /*! we assume the app already defines osp::vec types. Note those
      HAVE to be compatible with the data layouts used below.

      Note: this feature allows the application to use its own vector
      type library in the following way

      a) include your own vector library (say, ospcommon::vec3f etc,
      when using the ospcommon library)

      b) make sure the proper vec3f etc are defined in a osp:: namespace, e.g., using
      namespace osp {
         typedef ospcommon::vec3f vec3f;
      }

      c) defines OSPRAY_EXTERNAL_VECTOR_TYPES

      d) include ospray.h
      ! */
#else
  struct vec2f    { float x, y; };
  struct vec2i    { int x, y; };
  struct vec3f    { float x, y, z; };
  struct vec3fa   { float x, y, z; union { int a; unsigned u; float w; }; };
  struct vec3i    { int x, y, z; };
  struct vec4f    { float x, y, z, w; };
  struct box2i    { vec2i lower, upper; };
  struct box3f    { vec3f lower, upper; };
  struct linear3f { vec3f vx, vy, vz; };
  struct affine3f { linear3f l; vec3f p; };
#endif

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
} // ::osp

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
typedef osp::ManagedObject     *OSPObject;
typedef osp::PixelOp           *OSPPixelOp;

/* C++ DOES support default initializers */
#define OSP_DEFAULT_VAL(a) a

#else

typedef struct { float x, y; }                              osp_vec2f;
typedef struct { int x, y; }                                osp_vec2i;
typedef struct { float x, y, z; }                           osp_vec3f;
typedef struct { float x, y, z;
                 union { int a; unsigned u; float w; }; }   osp_vec3fa;
typedef struct { int x, y, z; }                             osp_vec3i;
typedef struct { float x, y, z, w; }                        osp_vec4f;
typedef struct { osp_vec2i lower, upper; }                  osp_box2i;
typedef struct { osp_vec3f lower, upper; }                  osp_box3f;
typedef struct { osp_vec3f vx, vy, vz; }                    osp_linear3f;
typedef struct { osp_linear3f l; osp_vec3f p; }             osp_affine3f;

/*! abstract object types. in C99, those are all the same because C99
  doesn't know inheritance, and we want to make sure that a
  OSPGeometry can still be passed to a function that expects a
  OSPObject, etc */
typedef struct _OSPManagedObject *OSPManagedObject,
  *OSPRenderer,
  *OSPCamera,
  *OSPFrameBuffer,
  *OSPModel,
  *OSPData,
  *OSPGeometry,
  *OSPMaterial,
  *OSPLight,
  *OSPVolume,
  *OSPTransferFunction,
  *OSPTexture2D,
  *OSPObject,
  *OSPPixelOp;

/* C99 does NOT support default initializers, so we use this macro
   to define them away */
#define OSP_DEFAULT_VAL(a) /* no default arguments on C99 */

#endif

#ifdef __cplusplus
extern "C" {
#endif

  //! initialize the ospray engine (for single-node user application)
  OSPRAY_INTERFACE void ospInit(int *ac, const char **av);

  //! \brief allows for switching the MPI mode btween collaborative, mastered, and independent
  OSPRAY_INTERFACE void ospdApiMode(OSPDApiMode);

  //! the 'lid to the pot' of ospdMpiInit().
  /*! does both an osp shutdown and an mpi shutdown for the mpi group
      created with ospdMpiInit */
  OSPRAY_INTERFACE
  void ospdMpiInit(int *ac, char ***av, OSPDRenderMode mode OSP_DEFAULT_VAL(=OSPD_Z_COMPOSITE));

  /*! the 'lid to the pot' of ospdMpiInit(). shuts down both ospray
      *and* the MPI layer created with ospdMpiInit */
  OSPRAY_INTERFACE void ospdMpiShutdown();

  //! load plugin 'name' from shard lib libospray_module_<name>.so
  /*! returns 0 if the module could be loaded, else it returns an error code > 0 */
  OSPRAY_INTERFACE int32_t ospLoadModule(const char *pluginName);

  //! use renderer to render a frame.
  /*! What input to use for rendering the frame is encoded in the
    renderer's parameters, typically in "world".
    return estimate of variance if framebuffer has VARIANCE buffer */
  OSPRAY_INTERFACE float ospRenderFrame(OSPFrameBuffer,
                                        OSPRenderer,
                                        const uint32_t frameBufferChannels OSP_DEFAULT_VAL(=OSP_FB_COLOR));

  //! create a new renderer of given type
  /*! return 'NULL' if that type is not known */
  OSPRAY_INTERFACE OSPRenderer ospNewRenderer(const char *type);

  //! create a new pixel op of given type
  /*! return 'NULL' if that type is not known */
  OSPRAY_INTERFACE OSPPixelOp ospNewPixelOp(const char *type);

  //! set a frame buffer's pixel op */
  OSPRAY_INTERFACE void ospSetPixelOp(OSPFrameBuffer, OSPPixelOp);

  //! create a new geometry of given type
  /*! return 'NULL' if that type is not known */
  OSPRAY_INTERFACE OSPGeometry ospNewGeometry(const char *type);

  //! let given renderer create a new material of given type
  OSPRAY_INTERFACE OSPMaterial ospNewMaterial(OSPRenderer, const char *type);

  //! let given renderer create a new light of given type
  OSPRAY_INTERFACE OSPLight ospNewLight(OSPRenderer, const char *type);

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
  OSPRAY_INTERFACE void ospRelease(OSPObject);

  //! assign given material to given geometry
  OSPRAY_INTERFACE void ospSetMaterial(OSPGeometry, OSPMaterial);

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
  OSPRAY_INTERFACE void ospAddVolume(OSPModel, OSPVolume);

  /*! \brief remove an existing volume from a model */
  OSPRAY_INTERFACE void ospRemoveVolume(OSPModel, OSPVolume);

  //! \brief create a new transfer function of given type
  /*! \detailed return 'NULL' if that type is not known */
  OSPRAY_INTERFACE OSPTransferFunction ospNewTransferFunction(const char * type);

  //! \brief create a new Texture2D with the given parameters
  /*! \detailed return 'NULL' if the texture could not be created with the given parameters */
#ifdef __cplusplus
  OSPRAY_INTERFACE OSPTexture2D ospNewTexture2D(const osp::vec2i &size,
                                                const OSPTextureFormat,
                                                void *source = NULL,
                                                const uint32_t textureCreationFlags = 0);
#else
  OSPRAY_INTERFACE OSPTexture2D ospNewTexture2D(const osp_vec2i *size,
                                                const OSPTextureFormat,
                                                void *source,
                                                const uint32_t textureCreationFlags);
#endif
  //! \brief clears the specified channel(s) of the frame buffer
  /*! \detailed clear the specified channel(s) of the frame buffer specified in 'whichChannels'

    if whichChannels & OSP_FB_COLOR != 0, clear the color buffer to '0,0,0,0'
    if whichChannels & OSP_FB_DEPTH != 0, clear the depth buffer to +inf
    if whichChannels & OSP_FB_ACCUM != 0, clear the accum buffer to 0,0,0,0, and reset accumID
  */
  OSPRAY_INTERFACE void ospFrameBufferClear(OSPFrameBuffer, const uint32_t frameBufferChannels);

  // -------------------------------------------------------
  /*! \defgroup ospray_data Data Buffer Handling

    \ingroup ospray_api

    \{
  */
  /*! create a new data buffer, with optional init data and control flags

    Valid flags that can be OR'ed together into the flags value:
    - OSP_DATA_SHARED_BUFFER: indicates that the buffer can be shared with the app.
    In this case the calling program guarantees that the 'init' pointer will remain
    valid for the duration that this data array is being used.
  */
  OSPRAY_INTERFACE OSPData ospNewData(size_t numItems,
                                      OSPDataType,
                                      const void *source OSP_DEFAULT_VAL(=NULL),
                                      const uint32_t dataCreationFlags OSP_DEFAULT_VAL(=0));

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
    eventually return. Valid values are OSP_FB_SRGBA, OSP_FB_RGBA8,
    OSP_FB_RGBA32F, and OSP_FB_NONE (note that
    OSP_FB_NONE is a perfectly reasonably choice for a framebuffer
    that will be used only internally, see notes below).
    The origin of the screen coordinate system is the lower left
    corner (as in OpenGL).

    \param channelFlags specifies which channels the frame buffer has,
    and is OR'ed together from the values OSP_FB_COLOR,
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
#ifdef __cplusplus
  OSPRAY_INTERFACE OSPFrameBuffer ospNewFrameBuffer(const osp::vec2i &size,
                                                    const OSPFrameBufferFormat format = OSP_FB_SRGBA,
                                                    const uint32_t frameBufferChannels = OSP_FB_COLOR);
#else
  OSPRAY_INTERFACE OSPFrameBuffer ospNewFrameBuffer(const osp_vec2i *size,
                                                    const OSPFrameBufferFormat,
                                                    const uint32_t frameBufferChannels);
#endif

  // \brief Set a given region of the volume to a given set of voxels
  /*! \detailed Given a block of voxels (of dimensions 'blockDim',
    located at the memory region pointed to by 'source', copy the
    given voxels into volume, at the region of addresses
    [regionCoords...regionCoord+regionSize].
  */
#ifdef __cplusplus
  OSPRAY_INTERFACE int ospSetRegion(/*! the object we're writing this block of pixels into */
                                    OSPVolume,
                                    /* points to the first voxel to be copies. The
                                       voxels at 'source' MUST have dimensions
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
#else
  OSPRAY_INTERFACE int ospSetRegion(OSPVolume,
                                    void *source,
                                    const osp_vec3i *regionCoords,
                                    const osp_vec3i *regionSize);
#endif


  /*! \brief free a framebuffer

    due to refcounting the frame buffer may not immediately be deleted
    at this time */
  OSPRAY_INTERFACE void ospFreeFrameBuffer(OSPFrameBuffer);

  /*! \brief map app-side content of a framebuffer (see \ref frame_buffer_handling) */
  OSPRAY_INTERFACE const void *ospMapFrameBuffer(OSPFrameBuffer,
                                                 const OSPFrameBufferChannel OSP_DEFAULT_VAL(=OSP_FB_COLOR));

  /*! \brief unmap a previously mapped frame buffer (see \ref frame_buffer_handling) */
  OSPRAY_INTERFACE void ospUnmapFrameBuffer(const void *mapped, OSPFrameBuffer);

  /*! \} */


  // -------------------------------------------------------
  /*! \defgroup ospray_params Object parameters.

    \ingroup ospray_api

    @{
  */
  /*! add a c-string (zero-terminated char *) parameter to another object */
  OSPRAY_INTERFACE void ospSetString(OSPObject, const char *id, const char *s);

  /*! add a object-typed parameter to another object */
  OSPRAY_INTERFACE void ospSetObject(OSPObject, const char *id, OSPObject other);

  /*! add a data array to another object */
  OSPRAY_INTERFACE void ospSetData(OSPObject, const char *id, OSPData);

  /*! add 1-float parameter to given object */
  OSPRAY_INTERFACE void ospSetf(OSPObject, const char *id, float x);

  /*! add 1-float parameter to given object */
  OSPRAY_INTERFACE void ospSet1f(OSPObject, const char *id, float x);

  /*! add 1-int parameter to given object */
  OSPRAY_INTERFACE void ospSet1i(OSPObject, const char *id, int32_t x);

  /*! add a 2-float parameter to a given object */
  OSPRAY_INTERFACE void ospSet2f(OSPObject, const char *id, float x, float y);

  /*! add 2-float parameter to given object */
  OSPRAY_INTERFACE void ospSet2fv(OSPObject, const char *id, const float *xy);

  /*! add a 2-int parameter to a given object */
  OSPRAY_INTERFACE void ospSet2i(OSPObject, const char *id, int x, int y);

  /*! add 2-int parameter to given object */
  OSPRAY_INTERFACE void ospSet2iv(OSPObject, const char *id, const int *xy);

  /*! add 3-float parameter to given object */
  OSPRAY_INTERFACE void ospSet3f(OSPObject, const char *id, float x, float y, float z);

  /*! add 3-float parameter to given object */
  OSPRAY_INTERFACE void ospSet3fv(OSPObject, const char *id, const float *xyz);

  /*! add 3-int parameter to given object */
  OSPRAY_INTERFACE void ospSet3i(OSPObject, const char *id, int x, int y, int z);

  /*! add 3-int parameter to given object */
  OSPRAY_INTERFACE void ospSet3iv(OSPObject, const char *id, const int *xyz);

  /*! add 4-float parameter to given object */
  OSPRAY_INTERFACE void ospSet4f(OSPObject, const char *id, float x, float y, float z, float w);

  /*! add 4-float parameter to given object */
  OSPRAY_INTERFACE void ospSet4fv(OSPObject, const char *id, const float *xyzw);

  /*! add untyped void pointer to object - this will *ONLY* work in local rendering!  */
  OSPRAY_INTERFACE void ospSetVoidPtr(OSPObject, const char *id, void *v);

#ifdef __cplusplus
  /*! add 2-float parameter to given object */
  OSPRAY_INTERFACE void ospSetVec2f(OSPObject, const char *id, const osp::vec2f &v);

  /*! add 2-int parameter to given object */
  OSPRAY_INTERFACE void ospSetVec2i(OSPObject, const char *id, const osp::vec2i &v);

  /*! add 3-float parameter to given object */
  OSPRAY_INTERFACE void ospSetVec3f(OSPObject, const char *id, const osp::vec3f &v);

  /*! add 3-int parameter to given object */
  OSPRAY_INTERFACE void ospSetVec3i(OSPObject, const char *id, const osp::vec3i &v);

  /*! add 4-float parameter to given object */
  OSPRAY_INTERFACE void ospSetVec4f(OSPObject, const char *id, const osp::vec4f &v);
#endif
  /*! @} end of ospray_params */

  // -------------------------------------------------------
  /*! \defgroup ospray_geometry Geometry Handling
    \ingroup ospray_api

    \{
  */

  /*! add an already created geometry to a model */
  OSPRAY_INTERFACE void ospAddGeometry(OSPModel, OSPGeometry);
  /*! \} end of ospray_trianglemesh */

  /*! \brief remove an existing geometry from a model */
  OSPRAY_INTERFACE void ospRemoveGeometry(OSPModel, OSPGeometry);


  /*! \brief create a new instance geometry that instantiates another
    model.  the resulting geometry still has to be added to another
    model via ospAddGeometry */
#ifdef __cplusplus
  OSPRAY_INTERFACE OSPGeometry ospNewInstance(OSPModel modelToInstantiate,
                                              const osp::affine3f &transform);
#else
  OSPRAY_INTERFACE OSPGeometry ospNewInstance(OSPModel modelToInstantiate,
                                              const osp_affine3f *transform);
#endif

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
  OSPRAY_INTERFACE void ospCommit(OSPObject);


#ifdef __cplusplus
  /*! \brief represents the result returned by an ospPick operation */
  typedef struct {
    osp::vec3f position; //< the position of the hit point (in world-space)
    bool hit;            //< whether or not a hit actually occured
  } OSPPickResult;

  /*! \brief returns the world-space position of the geometry seen at [0-1] normalized screen-space pixel coordinates (if any) */
  OSPRAY_INTERFACE void ospPick(OSPPickResult*, OSPRenderer, const osp::vec2f &screenPos);
#else
  typedef struct {
    osp_vec3f position; //< the position of the hit point (in world-space)
    int hit;            //< whether or not a hit actually occured
  } OSPPickResult;

  OSPRAY_INTERFACE void ospPick(OSPPickResult*, OSPRenderer, const osp_vec2f *screenPos);
#endif


  /*! \brief Samples the given volume at the provided world-space coordinates.

    \param results will be allocated by OSPRay and contain the
    resulting sampled values as floats. The application is
    responsible for freeing this memory. In case of error results
    will be NULL.

    This function is meant to provide a _small_ number of samples
    as needed for data probes, etc. in applications. It is not
    intended for large-scale sampling of volumes.
  */
#ifdef __cplusplus
  OSPRAY_INTERFACE void ospSampleVolume(float **results,
                                        OSPVolume,
                                        const osp::vec3f &worldCoordinates,
                                        const size_t count);
#else
  OSPRAY_INTERFACE void ospSampleVolume(float **results,
                                        OSPVolume,
                                        const osp_vec3f *worldCoordinates,
                                        const size_t count);
#endif

#ifdef __cplusplus
} // extern "C"
#endif

/*! \} */
