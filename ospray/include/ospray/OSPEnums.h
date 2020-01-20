// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// This header is shared with ISPC

#pragma once

// Log levels which can be set on a driver via "logLevel" parameter or
// "OSPRAY_LOG_LEVEL" environment variable
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

// enum representing different device properties
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

  // Unsigned character scalar and vector types.
  OSP_UCHAR = 2500,
  OSP_VEC2UC,
  OSP_VEC3UC,
  OSP_VEC4UC,
  OSP_BYTE = 2500, // XXX OSP_UCHAR, ISPC issue #1246
  OSP_RAW = 2500, // XXX OSP_UCHAR, ISPC issue #1246

  // Signed 16-bit integer scalar.
  OSP_SHORT = 3000,

  // Unsigned 16-bit integer scalar.
  OSP_USHORT = 3500,

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

  // Single precision floating point scalar and vector types.
  OSP_FLOAT = 6000,
  OSP_VEC2F,
  OSP_VEC3F,
  OSP_VEC4F,

  // Double precision floating point scalar type.
  OSP_DOUBLE = 7000,

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
  OSP_FB_ALBEDO = (1 << 5)
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

// OSPRay PerspectiveCamera stereo image modes
typedef enum
#if __cplusplus >= 201103L
    : uint8_t
#endif
{
  OSP_STEREO_NONE,
  OSP_STEREO_LEFT,
  OSP_STEREO_RIGHT,
  OSP_STEREO_SIDE_BY_SIDE,
  OSP_STEREO_UNKNOWN = 255
} OSPStereoMode;

// OSPRay Curves geometry types
typedef enum
#if __cplusplus >= 201103L
    : uint8_t
#endif
{
  OSP_ROUND,
  OSP_FLAT,
  OSP_RIBBON,
  OSP_UNKNOWN_CURVE_TYPE = 255
} OSPCurveType;

// OSPRay Curves geometry bases
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

// Subdivision modes
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
