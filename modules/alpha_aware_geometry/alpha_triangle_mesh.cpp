// ospray
#include "alpha_triangle_mesh.h"
#include "ospray/common/data.h"
#include "ospray/common/model.h"

//ispc

// embree
#include "embree2/rtcore.h"
#include "embree2/rtcore_scene.h"
#include "embree2/rtcore_geometry.h"
#include "../include/ospray/ospray.h"

//system
#include <algorithm>

#ifndef RTC_INVALID_ID
#define RTC_INVALID_ID RTC_INVALID_GEOMETRY_ID
#endif

namespace ospray {

  AlphaAwareTriangleMesh::AlphaAwareTriangleMesh()
    : TriangleMesh()
  {
    //TODO: ISPC side
    //this->ispcEquivalent = ispc::AlphaAwareTriangleMesh_create(this);
  }

  void AlphaAwareTriangleMesh::finalize(Model *model)
  {
    using namespace std;

    static int numPrints = 0;
    numPrints++;
    if (logLevel > 2) 
      if (numPrints == 5)
        cout << "(all future printouts for triangle mesh creation will be emitted)" << endl;
    
    if (logLevel > 2) 
      if (numPrints < 5)
        cout << "ospray: finalizing triangle mesh ..." << endl;
    Assert((eMesh == RTC_INVALID_ID) && "triangle mesh already built!?");
    
    Assert(model && "invalid model pointer");

    RTCScene embreeSceneHandle = model->embreeSceneHandle;
    vertexData = getParamData("vertex",getParamData("position"));
    normalData = getParamData("vertex.normal",getParamData("normal"));
    colorData  = getParamData("vertex.color",getParamData("color"));
    texcoordData = getParamData("vertex.texcoord",getParamData("texcoord"));
    indexData  = getParamData("index",getParamData("triangle"));
    prim_materialIDData = getParamData("prim.materialID");
    materialListData = getParamData("materialList");
    geom_materialID = getParam1i("geom.materialID",-1);

    Ref<Data> textureData = getParamData("alpha_texture", NULL);
    string alpha_type = getParamString("alpha_type", "alpha");
    string alpha_slot = getParamString("alpha_component", "a");

    Assert2(vertexData != NULL,
            "triangle mesh geometry does not have either 'position'"
            " or 'vertex' array");
    Assert2(indexData != NULL, 
            "triangle mesh geometry does not have either 'index'"
            " or 'triangle' array");
    Assert2(textureData != NULL,
            "triangle mesh geometry does not have a 'alpha_texture'");
    
    this->index = (vec3i*)indexData->data;
    this->vertex = (vec3fa*)vertexData->data;
    this->normal = normalData ? (vec3fa*)normalData->data : NULL;
    this->color  = colorData ? (vec4f*)colorData->data : NULL;
    this->texcoord = texcoordData ? (vec2f*)texcoordData->data : NULL;
    this->prim_materialID  = prim_materialIDData ? (uint32*)prim_materialIDData->data : NULL;
    this->materialList  = materialListData ? (ospray::Material**)materialListData->data : NULL;

    switch(alpha_slot[0]) {
      case 'x':
      case 'r':
        alphaComponent = R;
        break;
      case 'y':
      case 'g':
        alphaComponent = G;
        break;
      case 'z':
      case 'b':
        alphaComponent = B;
        break;
      case 'w':
      case 'a':
        alphaComponent = A;
        break;
      default:
        throw runtime_error("Unknown alpha slot provided to AlphaTirangleMesh");
    }

    transform(alpha_type.begin(), alpha_type.end(), alpha_type.begin(), ::tolower);
    if (alpha_type == "opacity") {
      alphaType = OPACITY;
    } else if (alpha_type == "alpha") {
      alphaType = ALPHA;
    } else {
      throw runtime_error("Unknown alpha representation provided to AlphaTriangleMesh");
    }

    if(alphaType == ALPHA)
      globalAlpha = getParam1f("alpha", 0.f);
    else
      globalAlpha = getParam1f("alpha", 1.f);

    map = (Texture2D*)getParamObject("alpha_map", NULL);
    cout << "Alpha map was null in AlphaAwareGeometry, global alpha setting will be used." << endl;
  }

  float AlphaAwareTriangleMesh::getAlpha(const vec4f &sample) const
  {
    float a;

    switch(alphaComponent) {
      case R:
        a = sample.x;
        break;
      case G:
        a = sample.y;
        break;
      case B:
        a = sample.z;
        break;
      case A:
        a = sample.w;
        break;
      default:
        throw std::runtime_error("FATAL: No alpha component slot was set for AlphaAwareTriangleMesh");
    }

    if(alphaType == OPACITY) a = 1.0f - a;

    return a;
  }

}
