#include "trianglemesh.ih"
#include "ospray/common/model.ih"

static void TriangleMesh_postIntersect(uniform Geometry *uniform geom,
                                       uniform Model *uniform model,
                                       varying DifferentialGeometry &dg,
                                       const varying Ray &ray,
                                       uniform int64 flags)
{
  uniform TriangleMesh *uniform THIS = (uniform TriangleMesh *uniform)geom;
  dg.geometry = geom;
  vec3f Ng = ray.Ng;
  vec3f Ns = Ng;

  dg.P = ray.org + ray.t * ray.dir;

  uniform vec3fa *uniform normal = THIS->normal;
#if 1
  if ((flags & DG_NS) && normal) {
    const vec3i index = THIS->index[ray.primID];
    Ns
      = (1.f-ray.u-ray.v) * make_vec3f(normal[index.x])
      + ray.u * make_vec3f(normal[index.y])
      + ray.v * make_vec3f(normal[index.z]);
    // print("normal interpolation!");
  }
#endif

  uniform vec4f *uniform color = THIS->color;
  if ((flags & DG_COLOR) && color) {
    const vec3i index = THIS->index[ray.primID];
    dg.color
      = (1.f-ray.u-ray.v) * (color[index.x])
      + ray.u * (color[index.y])
      + ray.v * (color[index.z]);
  }

  if (flags & DG_TEXCOORD && THIS->texcoord) {
    //calculate texture coordinate using barycentric coordinates
    const uniform vec3fa *uniform vertex = THIS->vertex;
    const uniform vec2f  *uniform texcoord = THIS->texcoord;
    assert(vertex); assert(texcoord);
    const vec3i idx = THIS->index[ray.primID];
    const vec3f p1 = make_vec3f(vertex[idx.x]) - dg.P;
    const vec3f p2 = make_vec3f(vertex[idx.y]) - dg.P;
    const vec3f p3 = make_vec3f(vertex[idx.z]) - dg.P;

    const float a = length(cross( make_vec3f(vertex[idx.x]) - make_vec3f(vertex[idx.y]),
                                  make_vec3f(vertex[idx.x]) - make_vec3f(vertex[idx.z])));
    const float a1 = length(cross(p2,p3)) / a;
    const float a2 = length(cross(p3,p1)) / a;
    const float a3 = length(cross(p1,p2)) / a;

    dg.st = texcoord[idx.x] * a1 + texcoord[idx.y] * a2 + texcoord[idx.z] * a3;
  } else {
    dg.st = make_vec2f(0.0f, 0.0f);
  }

  if (flags & DG_MATERIALID) {
    // print("trimesh:postisec\n");
    // print("primid %\n",ray.primID);
    if (THIS->prim_materialID) {
      dg.materialID = THIS->prim_materialID[ray.primID];
      // print("% : primID % -> matID %\n",THIS->materialList,ray.primID,dg.materialID);
    }
    else {
      dg.materialID = THIS->geom_materialID;
      // if (dg.materialID < 0) {
      //   print("trimesh has neither prim_mateiralID _NOR_ geom_materialID!!!!");
      //   print("TRIMESH: % %\n",THIS->geom_materialID,THIS->prim_materialID);
      // }
    }

    // print("bla\n");
    if( THIS->materialList) {
      //FIXME: crash here -- material pointers are bad
      // print("material INTO material list %\n",dg.materialID);
      dg.material = THIS->materialList[dg.materialID < 0 ? 0 : dg.materialID];
      // print("material fom material list %\n",dg.material);
    } else {
      // print("material FROM GEOM %\n",geom);
      dg.material = geom->material;
      // print("material fom geometry %\n",dg.material);
    }
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
                                     uniform vec2f  *uniform texcoord,
                                     uniform int32   geom_materialID,
                                     uniform Material *uniform *uniform materialList,
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
  mesh->texcoord     = texcoord;
  mesh->prim_materialID = prim_materialID;
  mesh->materialList = materialList;
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
  TriangleMesh_Constructor(mesh, cppEquivalent, 
                           NULL, 0, 0, NULL, NULL, NULL, NULL, NULL, -1, NULL, NULL);
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
                                      uniform vec2f  *uniform texcoord,
                                      uniform int32   geom_materialID,
                                      void *uniform _materialList,
                                      uniform uint32 *uniform prim_materialID
                                      )
{
  uniform TriangleMesh *uniform mesh = (uniform TriangleMesh *uniform)_mesh;
  uniform Model *uniform model = (uniform Model *uniform)_model;
  uniform Material *uniform *uniform materialList = (uniform Material *uniform *uniform)_materialList;
 TriangleMesh_Constructor(mesh,
                          mesh->geometry.cppEquivalent,
                          model,
                          geomID, 
                          numTriangles,
                          index,
                          vertex,
                          normal,
                          color, 
                          texcoord,
                          geom_materialID,
                          materialList,
                          prim_materialID);

 // if (prim_materialID) {
 //   for (int i=50;i<60;i++)
 //     print("list %, mat %\n",mesh->prim_materialID,mesh->prim_materialID[i]);
 // }

}

