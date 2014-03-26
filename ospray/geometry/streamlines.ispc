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
  uniform uint32 index  = geometry->index[primID];
  const uniform vec3f A = make_vec3f(geometry->vertex[index]);
  const uniform vec3f B = make_vec3f(geometry->vertex[index+1]);
  bbox.lower = min(A,B)-make_vec3f(geometry->radius);
  bbox.upper = max(A,B)+make_vec3f(geometry->radius);
}

void StreamLines_intersect(uniform StreamLines *uniform geometry,
                           varying Ray &ray,
                           uniform size_t primID)
{
  const uniform uint32 idx = geometry->index[primID];
  const uniform vec3fa A = geometry->vertex[idx];
  const uniform vec3fa B = geometry->vertex[idx+1];

  const vec3f P0 = ray.org;
  const vec3f u  = ray.dir;
  const vec3f Q0 = make_vec3f(A);
  const vec3f v  = make_vec3f(B)-make_vec3f(A);

  const vec3f w0 = P0-Q0;
  const float a = dot(u,u);
  const float b = dot(u,v);
  const float c = dot(v,v);
  const float d = dot(u,w0);
  const float e = dot(v,w0);
  
  const float t = (b*e - c*d) / (a*c - b*b);
  float f = (a*e - b*d) / (a*c - b*b); 
  if (f < 0.f) f = 0.f;
  if (f > 1.f) f = 1.f;
  const vec3f Q = Q0 + f * v;
  const vec3f P = P0 + t * u;

  float dist2 = dot(P-Q,P-Q);
  if (dist2 > geometry->radius*geometry->radius)
    return;

  if (t > ray.t)
    return;

  ray.primID = primID;
  ray.geomID = 0;
  ray.t = t;
  ray.Ng = P-Q;
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
  obj->radius = .001f; //radius;
  rtcSetUserData(scene,geomID,obj);
  rtcSetBoundsFunction(scene,geomID,(uniform RTCBoundsFunc)&StreamLines_bounds);
  rtcSetIntersectFunction(scene,geomID,(uniform RTCIntersectFuncVarying)&StreamLines_intersect);
}
