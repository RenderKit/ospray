// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// This header is shared with ISPC

#pragma once

// Log levels which can be set on a device via "logLevel" parameter
typedef enum
#if __cplusplus >= 201103L
    : uint32_t
#endif
{
  OSP_LOG_DEBUG = 1,
  OSP_LOG_INFO = 2,
  OSP_LOG_WARNING = 3,
  OSP_LOG_ERROR = 4,
  OSP_LOG_NONE = 5
} OSPLogLevel;

typedef enum
#if __cplusplus >= 201103L
    : uint32_t
#endif
{
  OSP_DEVICE_VERSION = 0,
  OSP_DEVICE_VERSION_MAJOR = 1,
  OSP_DEVICE_VERSION_MINOR = 2,
  OSP_DEVICE_VERSION_PATCH = 3,
  OSP_DEVICE_SO_VERSION = 4
} OSPDeviceProperty;

// An enum type that represensts the different data types represented in OSPRay
typedef enum
#if __cplusplus >= 201103L
    : uint32_t
#endif
{
  // Object reference type.
  OSP_DEVICE = 100,

  // Void pointer type.
  OSP_VOID_PTR = 200,

  // Booleans, same size as OSP_INT.
  OSP_BOOL = 250,

  // highest bit to represent objects/handles
  OSP_OBJECT = 0x8000000,

  // object subtypes
  OSP_DATA = 0x8000000 + 100,
  OSP_CAMERA,
  OSP_FRAMEBUFFER,
  OSP_FUTURE,
  OSP_GEOMETRIC_MODEL,
  OSP_GEOMETRY,
  OSP_GROUP,
  OSP_IMAGE_OPERATION,
  OSP_INSTANCE,
  OSP_LIGHT,
  OSP_MATERIAL,
  OSP_RENDERER,
  OSP_TEXTURE,
  OSP_TRANSFER_FUNCTION,
  OSP_VOLUME,
  OSP_VOLUMETRIC_MODEL,
  OSP_WORLD,

  // Pointer to a C-style NULL-terminated character string.
  OSP_STRING = 1500,

  // Character scalar type.
  OSP_CHAR = 2000,
  OSP_VEC2C,
  OSP_VEC3C,
  OSP_VEC4C,

  // Unsigned character scalar and vector types.
  OSP_UCHAR = 2500,
  OSP_VEC2UC,
  OSP_VEC3UC,
  OSP_VEC4UC,
  OSP_BYTE = 2500, // XXX OSP_UCHAR, ISPC issue #1246
  OSP_RAW = 2500, // XXX OSP_UCHAR, ISPC issue #1246

  // Signed 16-bit integer scalar.
  OSP_SHORT = 3000,
  OSP_VEC2S,
  OSP_VEC3S,
  OSP_VEC4S,

  // Unsigned 16-bit integer scalar.
  OSP_USHORT = 3500,
  OSP_VEC2US,
  OSP_VEC3US,
  OSP_VEC4US,

  // Signed 32-bit integer scalar and vector types.
  OSP_INT = 4000,
  OSP_VEC2I,
  OSP_VEC3I,
  OSP_VEC4I,

  // Unsigned 32-bit integer scalar and vector types.
  OSP_UINT = 4500,
  OSP_VEC2UI,
  OSP_VEC3UI,
  OSP_VEC4UI,

  // Signed 64-bit integer scalar and vector types.
  OSP_LONG = 5000,
  OSP_VEC2L,
  OSP_VEC3L,
  OSP_VEC4L,

  // Unsigned 64-bit integer scalar and vector types.
  OSP_ULONG = 5550,
  OSP_VEC2UL,
  OSP_VEC3UL,
  OSP_VEC4UL,

  // Half precision floating point scalar and vector types (IEEE 754
  // `binary16`).
  OSP_HALF = 5800,
  OSP_VEC2H,
  OSP_VEC3H,
  OSP_VEC4H,

  // Single precision floating point scalar and vector types.
  OSP_FLOAT = 6000,
  OSP_VEC2F,
  OSP_VEC3F,
  OSP_VEC4F,

  // Double precision floating point scalar type.
  OSP_DOUBLE = 7000,
  OSP_VEC2D,
  OSP_VEC3D,
  OSP_VEC4D,

  // Signed 32-bit integer N-dimensional box types
  OSP_BOX1I = 8000,
  OSP_BOX2I,
  OSP_BOX3I,
  OSP_BOX4I,

  // Single precision floating point N-dimensional box types
  OSP_BOX1F = 10000,
  OSP_BOX2F,
  OSP_BOX3F,
  OSP_BOX4F,

  // Transformation types
  OSP_LINEAR2F = 12000,
  OSP_LINEAR3F,
  OSP_AFFINE2F,
  OSP_AFFINE3F,

  OSP_QUATF,

  // Guard value.
  OSP_UNKNOWN = 9999999
} OSPDataType;

// OSPRay format constants for Texture creation
typedef enum
#if __cplusplus >= 201103L
    : uint32_t
#endif
{
  OSP_TEXTURE_RGBA8,
  OSP_TEXTURE_SRGBA,
  OSP_TEXTURE_RGBA32F,
  OSP_TEXTURE_RGB8,
  OSP_TEXTURE_SRGB,
  OSP_TEXTURE_RGB32F,
  OSP_TEXTURE_R8,
  OSP_TEXTURE_R32F,
  OSP_TEXTURE_L8,
  OSP_TEXTURE_RA8,
  OSP_TEXTURE_LA8,
  OSP_TEXTURE_RGBA16,
  OSP_TEXTURE_RGB16,
  OSP_TEXTURE_RA16,
  OSP_TEXTURE_R16,
  // Denotes an unknown texture format, so we can properly initialize parameters
  OSP_TEXTURE_FORMAT_INVALID = 255,
} OSPTextureFormat;

// Filter modes that can be set on 'texture2d' type OSPTexture
typedef enum
#if __cplusplus >= 201103L
    : uint32_t
#endif
{
  OSP_TEXTURE_FILTER_BILINEAR = 0, // default bilinear interpolation
  OSP_TEXTURE_FILTER_NEAREST // use nearest-neighbor interpolation
} OSPTextureFilter;

// Error codes returned by various API and callback functions
typedef enum
#if __cplusplus >= 201103L
    : uint32_t
#endif
{
  OSP_NO_ERROR = 0, // No error has been recorded
  OSP_UNKNOWN_ERROR = 1, // An unknown error has occurred
  OSP_INVALID_ARGUMENT = 2, // An invalid argument is specified
  OSP_INVALID_OPERATION =
      3, // The operation is not allowed for the specified object
  OSP_OUT_OF_MEMORY =
      4, // There is not enough memory left to execute the command
  OSP_UNSUPPORTED_CPU =
      5, // The CPU is not supported as it does not support SSE4.1
  OSP_VERSION_MISMATCH =
      6, // A module could not be loaded due to mismatching version
} OSPError;

// OSPRay format constants for Frame Buffer creation
typedef enum
#if __cplusplus >= 201103L
    : uint32_t
#endif
{
  OSP_FB_NONE, // framebuffer will not be mapped by application
  OSP_FB_RGBA8, // one dword per pixel: rgb+alpha, each one byte
  OSP_FB_SRGBA, // one dword per pixel: rgb (in sRGB space) + alpha, each one
                // byte
  OSP_FB_RGBA32F, // one float4 per pixel: rgb+alpha, each one float
} OSPFrameBufferFormat;

// OSPRay channel constants for Frame Buffer (can be OR'ed together)
typedef enum
#if __cplusplus >= 201103L
    : uint32_t
#endif
{
  OSP_FB_COLOR = (1 << 0),
  OSP_FB_DEPTH = (1 << 1),
  OSP_FB_ACCUM = (1 << 2),
  OSP_FB_VARIANCE = (1 << 3),
  OSP_FB_NORMAL = (1 << 4), // in world-space
  OSP_FB_ALBEDO = (1 << 5),
  OSP_FB_ID_PRIMITIVE = (1 << 6),
  OSP_FB_ID_OBJECT = (1 << 7),
  OSP_FB_ID_INSTANCE = (1 << 8)
} OSPFrameBufferChannel;

// OSPRay events which can be waited on via ospWait()
typedef enum
#if __cplusplus >= 201103L
    : uint32_t
#endif
{
  OSP_NONE_FINISHED = 0,
  OSP_WORLD_RENDERED = 10,
  OSP_WORLD_COMMITTED = 20,
  OSP_FRAME_FINISHED = 30,
  OSP_TASK_FINISHED = 100000
} OSPSyncEvent;

// OSPRay cell types definition for unstructured volumes, values are set to
// match VTK
typedef enum
#if __cplusplus >= 201103L
    : uint8_t
#endif
{
  OSP_TETRAHEDRON = 10,
  OSP_HEXAHEDRON = 12,
  OSP_WEDGE = 13,
  OSP_PYRAMID = 14,
  OSP_UNKNOWN_CELL_TYPE = 255
} OSPUnstructuredCellType;

// OSPRay camera stereo image modes
typedef enum
#if __cplusplus >= 201103L
    : uint8_t
#endif
{
  OSP_STEREO_NONE,
  OSP_STEREO_LEFT,
  OSP_STEREO_RIGHT,
  OSP_STEREO_SIDE_BY_SIDE,
  OSP_STEREO_TOP_BOTTOM,
  OSP_STEREO_UNKNOWN = 255
} OSPStereoMode;

typedef enum
#if __cplusplus >= 201103L
    : uint8_t
#endif
{
  OSP_SHUTTER_GLOBAL,
  OSP_SHUTTER_ROLLING_RIGHT,
  OSP_SHUTTER_ROLLING_LEFT,
  OSP_SHUTTER_ROLLING_DOWN,
  OSP_SHUTTER_ROLLING_UP,
  OSP_SHUTTER_UNKNOWN = 255
} OSPShutterType;

typedef enum
#if __cplusplus >= 201103L
    : uint8_t
#endif
{
  OSP_ROUND,
  OSP_FLAT,
  OSP_RIBBON,
  OSP_DISJOINT,
  OSP_UNKNOWN_CURVE_TYPE = 255
} OSPCurveType;

typedef enum
#if __cplusplus >= 201103L
    : uint8_t
#endif
{
  OSP_LINEAR,
  OSP_BEZIER,
  OSP_BSPLINE,
  OSP_HERMITE,
  OSP_CATMULL_ROM,
  OSP_UNKNOWN_CURVE_BASIS = 255
} OSPCurveBasis;

typedef enum
#if __cplusplus >= 201103L
    : uint8_t
#endif
{
  OSP_SUBDIVISION_NO_BOUNDARY,
  OSP_SUBDIVISION_SMOOTH_BOUNDARY,
  OSP_SUBDIVISION_PIN_CORNERS,
  OSP_SUBDIVISION_PIN_BOUNDARY,
  OSP_SUBDIVISION_PIN_ALL
} OSPSubdivisionMode;

// AMR Volume rendering methods
typedef enum
#if __cplusplus >= 201103L
    : uint8_t
#endif
{
  OSP_AMR_CURRENT,
  OSP_AMR_FINEST,
  OSP_AMR_OCTANT
} OSPAMRMethod;

// Filter modes for VDB and structured volumes, compatible with VKL
typedef enum
#if __cplusplus >= 201103L
    : uint32_t
#endif
{
  OSP_VOLUME_FILTER_NEAREST = 0, // treating voxel cell as constant
  OSP_VOLUME_FILTER_TRILINEAR = 100, // default trilinear interpolation
  OSP_VOLUME_FILTER_TRICUBIC = 200 // tricubic interpolation
} OSPVolumeFilter;

// VDB node data format
typedef enum
#if __cplusplus > 201103L
    : uint32_t
#endif
{
  OSP_VOLUME_FORMAT_TILE = 0, // node with no spatial variation.
  OSP_VOLUME_FORMAT_DENSE_ZYX // a dense grid of voxels in zyx layout
} OSPVolumeFormat;

// OSPRay pixel filter types
typedef enum
#if __cplusplus >= 201103L
    : uint8_t
#endif
{
  OSP_PIXELFILTER_POINT,
  OSP_PIXELFILTER_BOX,
  OSP_PIXELFILTER_GAUSS,
  OSP_PIXELFILTER_MITCHELL,
  OSP_PIXELFILTER_BLACKMAN_HARRIS
} OSPPixelFilterTypes;

// OSPRay light quantity types
typedef enum
#if __cplusplus >= 201103L
    : uint8_t
#endif
{
  OSP_INTENSITY_QUANTITY_RADIANCE, // unit W/sr/m^2
  OSP_INTENSITY_QUANTITY_IRRADIANCE, // unit W/m^2
  OSP_INTENSITY_QUANTITY_INTENSITY, // radiant intensity, unit W/sr
  OSP_INTENSITY_QUANTITY_POWER, // radiant flux, unit W
  OSP_INTENSITY_QUANTITY_SCALE, // linear scaling factor for the built-in type
  OSP_INTENSITY_QUANTITY_UNKNOWN
} OSPIntensityQuantity;
