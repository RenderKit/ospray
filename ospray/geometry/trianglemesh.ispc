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

  if ((flags & DG_NS) && THIS->normal) {
    print("HAS vertex normals!\n");
  }

  if (flags & DG_NORMALIZE) {
    Ng = normalize(Ng);
    Ns = normalize(Ns);
  }
  if (flags & DG_FACEFORWARD) {
    if (dot(ray.dir,Ng) >= 0.f) Ng = neg(Ng);
    if (dot(ray.dir,Ns) >= 0.f) Ns = neg(Ns);
  }
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
                                     uniform vec3fa *uniform color
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
                           NULL,0,0,NULL,NULL,NULL,NULL);
  return mesh;
}

export void *uniform TriangleMesh_set(void *uniform _mesh,
                                      void *uniform _model,
                                      uniform int32  geomID,
                                      uniform int32  numTriangles,
                                      uniform vec3i  *uniform index,
                                      uniform vec3fa *uniform vertex,
                                      uniform vec3fa *uniform normal,
                                      uniform vec3fa *uniform color
                                      )
{
  uniform TriangleMesh *uniform mesh = (uniform TriangleMesh *uniform)_mesh;
  uniform Model *uniform model = (uniform Model *uniform)_model;
  TriangleMesh_Constructor(mesh,mesh->geometry.cppEquivalent,model,geomID,
                           numTriangles,index,vertex,normal,color);
}

