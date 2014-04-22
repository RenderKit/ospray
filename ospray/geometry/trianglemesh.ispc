#include "trianglemesh.ih"

//! constructor for ispc-side TriangleMesh object
extern void TriangleMesh_Constructor(uniform TriangleMesh *uniform mesh,
                              void *uniform cppEquivalent,
                              uniform Model *uniform model,
                              uniform int32  geomID,
                              uniform int32  numTriangles,
                              uniform vec3i  *uniform index,
                              uniform vec3fa *uniform vertex)
{
  Geometry_Constructor(&mesh->geometry,cppEquivalent,model,geomID);
  mesh->numTriangles = numTriangles;
  mesh->index        = index;
  mesh->vertex       = vertex;
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
  TriangleMesh_Constructor(mesh,cppEquivalent,NULL,0,0,NULL,NULL);
  return mesh;
}

export void *uniform TriangleMesh_set(void *uniform _mesh,
                                      void *uniform _model,
                                      uniform int32  geomID,
                                      uniform int32  numTriangles,
                                      uniform vec3i  *uniform index,
                                      uniform vec3fa *uniform vertex)
{
  uniform TriangleMesh *uniform mesh = (uniform TriangleMesh *uniform)_mesh;
  uniform Model *uniform model = (uniform Model *uniform)_model;
  TriangleMesh_Constructor(mesh,mesh->geometry.cppEquivalent,model,geomID,numTriangles,index,vertex);
}

