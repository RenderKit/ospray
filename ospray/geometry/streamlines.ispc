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
  // print("bounds % : % % % - % % %\n",primID,
  //       bbox.lower.x,
  //       bbox.lower.y,
  //       bbox.lower.z,
  //       bbox.upper.x,
  //       bbox.upper.y,
  //       bbox.upper.z
  //       );
}

#if 1
inline float sqr(float f) { return f*f; }

void StreamLines_intersect(uniform StreamLines *uniform geometry,
                           varying Ray &ray,
                           uniform size_t primID)
{
  print("isec %\n",primID);
  const uniform uint32 idx = geometry->index[primID];
  const vec3f p_a = make_vec3f(geometry->vertex[idx]);
  const vec3f v_a_org = make_vec3f(geometry->vertex[idx+1]) - p_a;
  const float length = sqrt(dot(v_a_org,v_a_org));
  const vec3f v_a = v_a_org * rcp(length);

  const vec3f Avec = ray.dir-dot(ray.dir,v_a)*v_a;
  const vec3f delta_p = ray.org - p_a;
  const vec3f Cvec = delta_p-dot(delta_p,v_a)*v_a;
  const float A = dot(Avec,Avec);
  const float B = 2.f*dot(Avec,Cvec);
  const float C = dot(Cvec,Cvec)-sqr(geometry->radius);

  const float radical = B*B - 4.f*A*C;
  if (radical < 0.f || A==0.f) 
    return;
  const float sqrtf_radical = sqrt(radical);
  const float den = rcp(A+A);
  const float t0 = (-sqrtf_radical-B) * den;  
  if (t0 > ray.t0 && 
      t0 < ray.t) {
    const vec3f P = ray.org + t0*ray.dir;
    const float d = dot(P-p_a,v_a);
    // (iw): adding .1f*radius to stick at front and back to avoid
    // cracks. "proper" solution would be to add real spheres, but
    // this one's OK for starters.
    if (0.f <= d+.1f*geometry->radius && d-.1f*geometry->radius <= length) {
      ray.t = t0;
      ray.geomID = 0;
      ray.primID = primID;
      ray.Ng = P-(p_a+d*v_a);
      return;
    }
  }

  const float t1 = ( sqrtf_radical-B) * den;
  if (t1 > ray.t0 && t1 < ray.t) {
    const vec3f P = ray.org + t1*ray.dir;
    const float d = dot(P-p_a,v_a);
    // (iw): adding .1f*radius to stick at front and back to avoid
    // cracks. "proper" solution would be to add real spheres, but
    // this one's OK for starters.
    if (0.f <= d+.1f*geometry->radius && d-.1f*geometry->radius <= length) {
      ray.t = t1;
      ray.geomID = 0;
      ray.primID = primID;
      ray.Ng = P-(p_a+d*v_a);
      return;
    }
  }
}

#else
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
  
  float t = (b*e - c*d) / (a*c - b*b);
  if (t < ray.t0) t = ray.t0;
  if (t > ray.t)  t = ray.t;

  float f = (a*e - b*d) / (a*c - b*b); 
  if (f < 0.f) f = 0.f;
  if (f > 1.f) f = 1.f;
  const vec3f Q = Q0 + f * v;
  const vec3f P = P0 + t * u;

  float dist2 = dot(P-Q,P-Q);
  const float rad2 = geometry->radius*geometry->radius;
  if (dist2 > rad2)
    return;

  if (t > ray.t)
    return;

  ray.primID = primID;
  ray.geomID = 0;
  ray.t = t;
  ray.Ng = P-Q;
}
#endif

export void *uniform 
StreamLineGeometry_create(RTCScene        uniform scene,
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
  print("radius in ispc: %\n",radius);
  rtcSetUserData(scene,geomID,obj);
  rtcSetBoundsFunction(scene,geomID,(uniform RTCBoundsFunc)&StreamLines_bounds);
  rtcSetIntersectFunction(scene,geomID,(uniform RTCIntersectFuncVarying)&StreamLines_intersect);
}
