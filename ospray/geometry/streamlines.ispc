// ospray
#include "common/math/vec.ih"
#include "common/math/bbox.ih"
#include "common/ray.ih"
// embree
#include "embree2/rtcore.isph"
#include "embree2/rtcore_scene.isph"
#include "embree2/rtcore_geometry_user.isph"

struct StreamLines {
  RTCScene        scene;
  uint32          geomID;
  void           *cppEquivalent;
  float           radius;
  uniform vec3fa *vertex;
  int32           numVertices;
  uniform uint32 *index;
  int32           numSegments;
};

void StreamLines_bounds(uniform StreamLines *uniform geometry,
                        uniform size_t primID,
                        uniform box3fa &bbox)
{
  uniform uint32 index = geometry->index[primID];
  const uniform vec3f A = make_vec3f(geometry->vertex[index]);
  const uniform vec3f B = make_vec3f(geometry->vertex[index+1]);
  bbox.lower = min(A,B)-make_vec3f(geometry->radius);
  bbox.upper = min(A,B)-make_vec3f(geometry->radius);
}

void StreamLines_intersect()
{
}

export void *uniform 
ispc_createStreamLineGeometry(RTCScene        uniform scene,
                              void           *uniform cppEquivalent,
                              float           uniform radius,
                              uniform vec3fa *uniform vertex,
                              int32           uniform numVertices,
                              uniform uint32 *uniform index,
                              int32           uniform numSegments)
{
  uniform uint32 geomID = rtcNewUserGeometry(scene,numSegments);
  uniform StreamLines *uniform obj = uniform new uniform StreamLines;
  obj->cppEquivalent = cppEquivalent;
  obj->vertex = vertex;
  obj->index = index;
  obj->numSegments = numSegments;
  obj->numVertices = numVertices;
  obj->geomID = geomID;
  obj->scene = scene;
  obj->radius = radius;
  rtcSetUserData(scene,geomID,obj);
  rtcSetBoundsFunction(scene,geomID,(uniform RTCBoundsFunc)&StreamLines_bounds);
  rtcSetIntersectFunction(scene,geomID,(uniform RTCIntersectFuncVarying)&StreamLines_intersect);
}
