// ospray
#include "common/math/vec.ih"
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

void StreamLines_bounds()
{
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
