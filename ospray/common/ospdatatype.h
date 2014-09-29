//
//                 INTEL CORPORATION PROPRIETARY INFORMATION
//
//    This software is supplied under the terms of a license agreement or
//    nondisclosure agreement with Intel Corporation and may not be copied
//    or disclosed except in accordance with the terms of that agreement.
//    Copyright (C) 2014 Intel Corporation. All Rights Reserved.
//

#ifndef OSP_DATA_TYPE_H
#define OSP_DATA_TYPE_H

typedef enum {

    //! Void pointer type.
    OSP_VOID_PTR,

    //! Object reference type.
    OSP_OBJECT,

    //! Object reference subtypes.
    OSP_CAMERA,
    OSP_DATA,
    OSP_FRAMEBUFFER,
    OSP_GEOMETRY,
    OSP_LIGHT,
    OSP_MATERIAL,
    OSP_MODEL,
    OSP_RENDERER,
    OSP_TEXTURE,
    OSP_TRANSFER_FUNCTION,
    OSP_VOLUME,

    //! Pointer to a C-style NULL-terminated character string.
    OSP_STRING,

    //! Character scalar type.
    OSP_CHAR,

    //! Unsigned character scalar and vector types.
    OSP_UCHAR, OSP_UCHAR2, OSP_UCHAR3, OSP_UCHAR4,

    //! Signed integer scalar and vector types.
    OSP_INT, OSP_INT2, OSP_INT3, OSP_INT4,

    //! Unsigned integer scalar and vector types.
    OSP_UINT, OSP_UINT2, OSP_UINT3, OSP_UINT4,

    //! Single precision floating point scalar and vector types.
    OSP_FLOAT, OSP_FLOAT2, OSP_FLOAT3, OSP_FLOAT4, OSP_FLOAT3A,

    //! Guard value.
    OSP_UNKNOWN,

} OSPDataType;

inline size_t sizeOf(OSPDataType type) {

    switch (type) {
    case OSP_VOID_PTR:  return sizeof(void *);
    case OSP_OBJECT:    return sizeof(void *);
    case OSP_CHAR:      return sizeof(int8);
    case OSP_UCHAR:     return sizeof(uint8);
    case OSP_UCHAR2:    return sizeof(embree::Vec2<uint8>);
    case OSP_UCHAR3:    return sizeof(embree::Vec3<uint8>);
    case OSP_UCHAR4:    return sizeof(uint32);
    case OSP_INT:       return sizeof(int32);
    case OSP_INT2:      return sizeof(embree::Vec2<int32>);
    case OSP_INT3:      return sizeof(embree::Vec3<int32>);
    case OSP_INT4:      return sizeof(embree::Vec4<int32>);
    case OSP_UINT:      return sizeof(uint32);
    case OSP_UINT2:     return sizeof(embree::Vec2<uint32>);
    case OSP_UINT3:     return sizeof(embree::Vec3<uint32>);
    case OSP_UINT4:     return sizeof(embree::Vec4<uint32>);
    case OSP_FLOAT:     return sizeof(float);
    case OSP_FLOAT2:    return sizeof(embree::Vec2<float>);
    case OSP_FLOAT3:    return sizeof(embree::Vec3<float>);
    case OSP_FLOAT4:    return sizeof(embree::Vec4<float>);
    case OSP_FLOAT3A:   return sizeof(embree::Vec3fa);
    default: break;
    };

    std::stringstream error;
    error << __FILE__ << ":" << __LINE__ << ": unknown OSPDataType " << (int)type;
    throw std::runtime_error(error.str());

}

#endif // OSP_DATA_TYPE_H

