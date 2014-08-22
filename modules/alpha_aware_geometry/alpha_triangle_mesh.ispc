//ospray
#include "alpha_triangle_mesh.ih"
#include "ospray/common/model.ih"
#include "ospray/texture/texture2d.ih"

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
  assert(ray.geomID >= 0);
  uniform AlphaAwareTriangleMesh *uniform THIS = (uniform AlphaAwareTriangleMesh *uniform)geom;
  dg.geometry = geom;
  vec3f Ng = ray.Ng;
  vec3f Ns = Ng;

  dg.P = ray.org + ray.t * ray.dir;

  uniform vec3fa *uniform normal = THIS->base.normal;
#if 1
  if ((flags & DG_NS) && normal) {
    const vec3i index = THIS->base.index[ray.primID];
    Ns
      = (1.f-ray.u-ray.v) * make_vec3f(normal[index.x])
      + ray.u * make_vec3f(normal[index.y])
      + ray.v * make_vec3f(normal[index.z]);
  }
#endif

  if ((flags & DG_COLOR)) {
    uniform vec4f *uniform color = THIS->base.color;
    if (color) {
      const vec3i index = THIS->base.index[ray.primID];
      dg.color
        = (1.f-ray.u-ray.v) * (color[index.x])
        + ray.u * (color[index.y])
        + ray.v * (color[index.z]);
    }
    else
      dg.color = make_vec4f(1.f);
  }

  if (flags & DG_TEXCOORD && THIS->base.texcoord) {
    //calculate texture coordinate using barycentric coordinates
    const uniform vec3fa *uniform vertex = THIS->base.vertex;
    const uniform vec2f  *uniform texcoord = THIS->base.texcoord;
    // assert(vertex); assert(texcoord);
    const vec3i index = THIS->base.index[ray.primID];
    dg.st
      = (1.f-ray.u-ray.v) * (texcoord[index.x])
      + ray.u * (texcoord[index.y])
      + ray.v * (texcoord[index.z]);
  } else {
    dg.st = make_vec2f(0.0f, 0.0f);
  }

  if (flags & DG_MATERIALID) {
    if (THIS->base.prim_materialID) {
      dg.materialID = THIS->base.prim_materialID[ray.primID];
    }
    else {
      dg.materialID = THIS->base.geom_materialID;
    }

    if( THIS->base.materialList) {
      //FIXME: crash here -- material pointers are bad
      //     print("materialID INTO material list %\n",dg.materialID);
      //print("materialList = %\n",THIS->materialList);
      //print("materialList[0] = %\n",THIS->materialList[0]);
      dg.material = THIS->base.materialList[dg.materialID < 0 ? 0 : dg.materialID];
      //      print("material fom material list %\n",dg.material);
    } else {
      //    print("material FROM GEOM %\n",geom);
      dg.material = geom->material;
      //   print("material fom geometry %\n",dg.material);
    }

  }

  dg.Ns = Ns;
  dg.Ng = Ng;
}

inline static float AlphaAwareTriangleMesh_getAlpha(uniform AlphaAwareTriangleMesh *uniform mesh, const DifferentialGeometry &dg)
{
  assert(mesh);

  float alpha = 0.f;
  if(mesh->maps && mesh->maps[dg.materialID]) {
    Texture2D *map = mesh->maps[dg.materialID];
    foreach_unique(m in map) {
      vec4f sample = get4f(m, dg.st);
      switch(mesh->alphaComponent) {
        case R:
          alpha = sample.x;
          break;
        case G:
          alpha = sample.y;
          break;
        case B:
          alpha = sample.z;
          break;
        case A:
          alpha = sample.w;
          break;
        default:
          alpha = 0.f;
          break;
      };
    }
  } else if (mesh->globalAlphas) {
    alpha = mesh->globalAlphas[dg.materialID];
  } else {
  }

  //handle representation change from opacity to alpha if necessary
  if(mesh->alphaType == OPACITY) alpha = 1.0f - alpha;
  return alpha;
}

#ifdef OSPRAY_INTERSECTION_FILTER
inline bool SIMILAR(float a, float b)
{
  const float EPSILON = .00001f;
  return absf(a - b) <= EPSILON;
}

static void intersectionFilter(void *uniform ptr, /*!< pointer to user data */
      varying Ray &ray   /*!< intersection to filter */)
{
  uniform AlphaAwareTriangleMesh *uniform mesh = (uniform AlphaAwareTriangleMesh *uniform)ptr;
  if (ray.geomID < 0) return;
  DifferentialGeometry dg;
  AlphaAwareTriangleMesh_postIntersect((uniform Geometry *uniform)mesh,
                                        mesh->base.geometry.model,
                                        dg,
                                        ray,
                                        DG_MATERIALID | DG_TEXCOORD);
  if(dg.materialID > -1) {
    float alpha = AlphaAwareTriangleMesh_getAlpha(mesh, dg);
    if (SIMILAR(1.0f, alpha)){
      ray.geomID = -1;
      return;
    }
  }

  //if(ray.intersectionFilter)
  //  ray.intersectionFilter((uniform Geometry *uniform)ptr, ray);
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
                                        uniform float *uniform alphas,
                                        uniform Texture2D *uniform *uniform maps)
{
  TriangleMesh_Constructor(&mesh->base, cppEquivalent, model, geomID, numTriangles,
                           index, vertex, normal, color, texcoord, geom_materialID,
                           materialList, prim_materialID);
  mesh->base.geometry.postIntersect = &AlphaAwareTriangleMesh_postIntersect;
  mesh->globalAlphas = alphas;
  mesh->maps = maps;
  mesh->alphaComponent = (AlphaComponent)alphaComponent;
  mesh->alphaType = (AlphaType)alphaType;
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
                                     NULL,   //alpha
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
                                                uniform float *uniform alphas,
                                                void *uniform _maps)
{
  uniform AlphaAwareTriangleMesh *uniform mesh = (uniform AlphaAwareTriangleMesh *uniform)_mesh;
  uniform Model *uniform model = (uniform Model *uniform)_model;
  uniform Material *uniform *uniform materialList = (uniform Material *uniform *uniform)_materialList;
  uniform Texture2D *uniform *uniform maps = (uniform Texture2D *uniform *uniform)_maps;

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
                                     alphas,
                                     maps);

#ifdef OSPRAY_INTERSECTION_FILTER
 rtcSetUserData(model->embreeSceneHandle,geomID,mesh);
 rtcSetIntersectionFilterFunction(model->embreeSceneHandle,geomID,
                                  (uniform RTCFilterFuncVarying)&intersectionFilter);
#endif
}
