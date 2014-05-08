#include "trianglemesh.ih"

static void TriangleMesh_postIntersect(uniform Geometry *uniform geom,
                                       uniform Model *uniform model,
                                       varying DifferentialGeometry &dg,
                                       const varying Ray &ray,
                                       uniform int64 flags)
{
  uniform TriangleMesh *uniform THIS = (uniform TriangleMesh *uniform)geom;
  dg.geometry = geom;
  dg.material = geom->material;
  vec3f Ng = ray.Ng;
  vec3f Ns = Ng;

  uniform vec3fa *uniform normal = THIS->normal;
  if ((flags & DG_NS) && normal) {
    const vec3i index = THIS->index[ray.primID];
    Ns
      = (1.f-ray.u-ray.v) * make_vec3f(normal[index.x])
      + ray.u * make_vec3f(normal[index.y])
      + ray.v * make_vec3f(normal[index.z]);
  }

  uniform vec4f *uniform color = THIS->color;
  if ((flags & DG_NS) && color) {
    const vec3i index = THIS->index[ray.primID];
    dg.color
      = (1.f-ray.u-ray.v) * (color[index.x])
      + ray.u * (color[index.y])
      + ray.v * (color[index.z]);
  }

  if (flags & DG_MATERIALID) {
    if (THIS->prim_materialID) 
      dg.materialID = THIS->prim_materialID[ray.primID];
    else
      dg.materialID = THIS->geom_materialID;
  }

  if (flags & DG_NORMALIZE) {
    Ng = normalize(Ng);
    Ns = normalize(Ns);
  }
  if (flags & DG_FACEFORWARD) {
    if (dot(ray.dir,Ng) >= 0.f) Ng = neg(Ng);
    if (dot(ray.dir,Ns) >= 0.f) Ns = neg(Ns);
  }
  dg.Ns = Ns;
  dg.Ng = Ng;
}


//! constructor for ispc-side TriangleMesh object
extern void TriangleMesh_Constructor(uniform TriangleMesh *uniform mesh,
                                     void *uniform cppEquivalent,
                                     uniform Model *uniform model,
                                     uniform int32  geomID,
                                     uniform int32  numTriangles,
                                     uniform vec3i  *uniform index,
                                     uniform vec3fa *uniform vertex,
                                     uniform vec3fa *uniform normal,
                                     uniform vec4f  *uniform color,
                                     uniform int32   geom_materialID,
                                     uniform uint32 *uniform prim_materialID
                                     )
{
  Geometry_Constructor(&mesh->geometry,cppEquivalent,
                       TriangleMesh_postIntersect,
                       model,geomID,
                       mesh->geometry.material);
  mesh->numTriangles = numTriangles;
  mesh->index        = index;
  mesh->vertex       = vertex;
  mesh->normal       = normal;
  mesh->color        = color;
  mesh->prim_materialID = prim_materialID;
  mesh->geom_materialID = geom_materialID;
}


//! destructor for ispc-side TriangleMesh object
extern void TriangleMesh_Destructor(uniform TriangleMesh *uniform mesh)
{ 
  Geometry_Destructor(&mesh->geometry); 
}

//! free and properly destroy the given mesh geometry
export void TriangleMesh_destroy(void *uniform _mesh)
{
  uniform TriangleMesh *uniform mesh = (uniform TriangleMesh *uniform)_mesh;
  TriangleMesh_Destructor(mesh);
  delete mesh;
}

export void *uniform TriangleMesh_create(void *uniform cppEquivalent)
{
  uniform TriangleMesh *uniform mesh = uniform new uniform TriangleMesh;
  TriangleMesh_Constructor(mesh,cppEquivalent,
                           NULL,0,0,NULL,NULL,NULL,NULL,-1,NULL);
  return mesh;
}

export void *uniform TriangleMesh_set(void *uniform _mesh,
                                      void *uniform _model,
                                      uniform int32  geomID,
                                      uniform int32  numTriangles,
                                      uniform vec3i  *uniform index,
                                      uniform vec3fa *uniform vertex,
                                      uniform vec3fa *uniform normal,
                                      uniform vec4f  *uniform color,
                                      uniform int32   geom_materialID,
                                      uniform uint32 *uniform prim_materialID
                                      )
{
  uniform TriangleMesh *uniform mesh = (uniform TriangleMesh *uniform)_mesh;
  uniform Model *uniform model = (uniform Model *uniform)_model;
  TriangleMesh_Constructor(mesh,mesh->geometry.cppEquivalent,model,geomID,
                           numTriangles,index,vertex,normal,color,
                           geom_materialID,prim_materialID);
}

