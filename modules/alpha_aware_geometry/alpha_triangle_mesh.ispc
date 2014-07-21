//ospray
#include "alpha_triangle_mesh.ih"
#include "ospray/common/model.ih"

//embree
#include "embree2/rtcore.isph"
#include "embree2/rtcore_geometry.isph"
#include "embree2/rtcore_geometry_user.isph"

static void AlphaAwareTriangleMesh_postIntersect(uniform Geometry *uniform geom,
                                                 uniform Model *uniform model,
                                                 varying DifferentialGeometry &dg,
                                                 const varying Ray &ray,
                                                 uniform int64 flags)
{
  assert(0);
}

#ifdef OSPRAY_INTERSECTION_FILTER
  void intersectionFilter(void *uniform ptr, /*!< pointer to user data */
                          varying Ray &ray   /*!< intersection to filter */)
  {
    uniform Geometry *uniform geom = (uniform Geometry *uniform)ptr;
    if (ray.intersectionFilter) {
      ray.intersectionFilter(geom, (varying Ray&)ray);
    }
  }
#endif

void AlphaAwareTriangleMesh_Constructor(uniform AlphaAwareTriangleMesh *uniform mesh,
                                        void *uniform cppEquivalent,
                                        uniform Model *uniform model,
                                        uniform int32 geomID,
                                        uniform int32 numTriangles,
                                        uniform vec3i  *uniform index,
                                        uniform vec3fa *uniform vertex,
                                        uniform vec3fa *uniform normal,
                                        uniform vec4f  *uniform color,
                                        uniform vec2f  *uniform texcoord,
                                        uniform int32   geom_materialID,
                                        uniform Material *uniform *uniform materialList,
                                        uniform uint32 *uniform prim_materialID,
                                        uniform uint32 alphaComponent,
                                        uniform uint32 alphaType,
                                        uniform float alpha,
                                        uniform Texture2D *uniform map)
{
  TriangleMesh_Constructor(&mesh->base, cppEquivalent, model, geomID, numTriangles,
                           index, vertex, normal, color, texcoord, geom_materialID,
                           materialList, prim_materialID);
  mesh->globalAlpha = alpha;
  mesh->map = map;
  mesh->alphaComponent = (AlphaComponent)alphaComponent;
  mesh->alphaType = (AlphaType)alphaType;

#ifdef OSPRA_INTERSECTION_FILTER
  rtcSetUserData(model->embreeSceneHandle, geomID, mesh);
  rtcSetIntersectionFilterFunction(model->embreeSceneHandle, geomID, (uniform RTCFilterFuncVarying)&intersectionFilter);
#endif
}

extern void AlphaAwareTriangleMesh_Destructor(uniform AlphaAwareTriangleMesh *uniform mesh)
{
  TriangleMesh_Destructor(&mesh->base); //calls the geometry constructor already
}

export void AlphaAwareTriangleMesh_destroy(void *uniform _mesh)
{
  uniform AlphaAwareTriangleMesh *uniform mesh = (uniform AlphaAwareTriangleMesh *uniform)_mesh;
  AlphaAwareTriangleMesh_Destructor(mesh);
  delete mesh;
}

export void *uniform AlphaAwareTriangleMesh_create(void *uniform cppEquivalent)
{
  uniform AlphaAwareTriangleMesh *uniform mesh = uniform new uniform AlphaAwareTriangleMesh;
  AlphaAwareTriangleMesh_Constructor(mesh,
                                     cppEquivalent,
                                     NULL,  //model
                                     0,     //geomID
                                     0,     //numTriangles
                                     NULL,  //index
                                     NULL,  //vertex
                                     NULL,  //normal
                                     NULL,  //color
                                     NULL,  //texcoord
                                     -1,    //geom_materialID
                                     NULL,  //materialList
                                     NULL,  //prim_materialID
                                     0,     //alphaComponent
                                     0,     //alphaType
                                     0.f,   //alpha
                                     NULL); //map
  return mesh;
}

export void *uniform AlphaAwareTriangleMesh_set(void *uniform _mesh,
                                                void *uniform _model,
                                                uniform int32 geomID,
                                                uniform int32 numTriangles,
                                                uniform vec3i  *uniform index,
                                                uniform vec3fa *uniform vertex,
                                                uniform vec3fa *uniform normal,
                                                uniform vec4f  *uniform color,
                                                uniform vec2f  *uniform texcoord,
                                                uniform int32   geom_materialID,
                                                void *uniform _materialList,
                                                uniform uint32 *uniform prim_materialID,
                                                uniform uint32 alphaComponent,
                                                uniform uint32 alphaType,
                                                uniform float alpha,
                                                void *uniform _map)
{
  uniform AlphaAwareTriangleMesh *uniform mesh = (uniform AlphaAwareTriangleMesh *uniform)_mesh;
  uniform Model *uniform model = (uniform Model *uniform)_model;
  uniform Material *uniform *uniform materialList = (uniform Material *uniform *uniform)_materialList;
  uniform Texture2D *uniform map = (uniform Texture2D *uniform)_map;

  AlphaAwareTriangleMesh_Constructor(mesh,
                                     mesh->base.geometry.cppEquivalent,
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
                                     prim_materialID,
                                     alphaComponent,
                                     alphaType,
                                     alpha,
                                     map);

#ifdef OSPRAY_INTERSECTION_FILTER
 rtcSetUserData(model->embreeSceneHandle,geomID,mesh);
 rtcSetIntersectionFilterFunction(model->embreeSceneHandle,geomID,
                                  (uniform RTCFilterFuncVarying)&intersectionFilter);
#endif
}
