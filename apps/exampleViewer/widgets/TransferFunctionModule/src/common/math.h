#pragma once
#ifdef TFNMODULE_EXTERNAL_VECTOR_TYPES
/*! we assume the app already defines osp::vec types. Note those
  HAVE to be compatible with the data layouts used below.
  Note: this feature allows the application to use its own vector
  type library in the following way
  a) include your own vector library (say, ospcommon::vec3f etc, when using the ospcommon library)
  b) make sure the proper vec3f etc are defined in a osp:: namespace, e.g., using
  namespace tfn {
    typedef ospcommon::vec3f vec3f;
  }
  c) defines OSPRAY_EXTERNAL_VECTOR_TYPES
  d) include vec.h
  ! */
#else
namespace tfn {
    struct vec2f    { float x, y; };
    struct vec2i    { int x, y; };
    struct vec3f    { float x, y, z; };
    struct vec3fa   { float x, y, z; union { int a; unsigned u; float w; }; };
    struct vec3i    { int x, y, z; };
    struct vec4f    { float x, y, z, w; };
    struct box2i    { vec2i lower, upper; };
    struct box3i    { vec3i lower, upper; };
    struct box3f    { vec3f lower, upper; };
    struct linear3f { vec3f vx, vy, vz; };
    struct affine3f { linear3f l; vec3f p; };
};
#endif
