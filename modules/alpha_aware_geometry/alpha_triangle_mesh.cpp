// ospray
#include "alpha_triangle_mesh.h"
#include "ospray/common/Data.h"
#include "ospray/common/model.h"
#include "ospray/texture/texture2d.h"

//ispc
#include "alpha_triangle_mesh_ispc.h"

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
    this->ispcEquivalent = ispc::AlphaAwareTriangleMesh_create(this);
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


    Assert2(vertexData != NULL,
            "triangle mesh geometry does not have either 'position'"
            " or 'vertex' array");
    Assert2(indexData != NULL, 
            "triangle mesh geometry does not have either 'index'"
            " or 'triangle' array");
    
    this->index = (vec3i*)indexData->data;
    this->vertex = (vec3fa*)vertexData->data;
    this->normal = normalData ? (vec3fa*)normalData->data : NULL;
    this->color  = colorData ? (vec4f*)colorData->data : NULL;
    this->texcoord = texcoordData ? (vec2f*)texcoordData->data : NULL;
    this->prim_materialID  = prim_materialIDData ? (uint32*)prim_materialIDData->data : NULL;
    this->materialList  = materialListData ? (ospray::Material**)materialListData->data : NULL;

    if (materialList) {
      num_materials = materialListData->numItems;
      ispcMaterialPtrs = new void*[num_materials];
      for (int i = 0; i < num_materials; i++) {
        this->ispcMaterialPtrs[i] = this->materialList[i]->getIE();
      }
    } else {
      this->ispcMaterialPtrs = NULL;
    }

    size_t numTris  = -1;
    size_t numVerts = -1;
    switch (indexData->type) {
    case OSP_INT:  numTris = indexData->size() / 3; break;
    case OSP_UINT: numTris = indexData->size() / 3; break;
    case OSP_INT3:  numTris = indexData->size(); break;
    case OSP_UINT3: numTris = indexData->size(); break;
    default:
      throw std::runtime_error("unsupported trianglemesh.index data type");
    }

    switch (vertexData->type) {
    case OSP_FLOAT:   numVerts = vertexData->size() / 4; break;
    case OSP_FLOAT3A:  numVerts = vertexData->size(); break;
    default:
      throw std::runtime_error("unsupported trianglemesh.vertex data type");
    }

    eMesh = rtcNewTriangleMesh(embreeSceneHandle,RTC_GEOMETRY_STATIC,
                               numTris,numVerts);

    rtcSetBuffer(embreeSceneHandle,eMesh,RTC_VERTEX_BUFFER,
                 (void*)this->vertex,0,
                 sizeOf(vertexData->type));
    rtcSetBuffer(embreeSceneHandle,eMesh,RTC_INDEX_BUFFER,
                 (void*)this->index,0,
                 sizeOf(indexData->type));

    box3f bounds = embree::empty;
    
    for (int i=0;i<vertexData->size();i++) 
      bounds.extend(vertex[i]);

    if (logLevel > 2) 
      if (numPrints < 5) {
        cout << "  created triangle mesh (" << numTris << " tris "
             << ", " << numVerts << " vertices)" << endl;
        cout << "  mesh bounds " << bounds << endl;
      } 

    string alpha_type = getParamString("alpha_type", "alpha");
    string alpha_slot = getParamString("alpha_component", "a");

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

    alphaMapData = getParamData("alpha_maps");
    alphaData = getParamData("alphas");

    mapPointers = alphaMapData ? (Texture2D**)alphaMapData->data : NULL;
    globalAlphas = alphaData ? (float*)alphaData->data : NULL;

    if(alphaType == OPACITY)
      for(int i = 0; i < num_materials; i++)
        globalAlphas[i] = 1.0f - globalAlphas[i];

    if(alphaMapData) {
      ispcMapPointers = new void*[num_materials];
      for(int i = 0; i < num_materials; i++) {
        if(mapPointers[i]) {
          ispcMapPointers[i] = mapPointers[i]->getIE();
        } else {
          ispcMapPointers[i] = NULL;
        }
      }
    }

    ispc::AlphaAwareTriangleMesh_set(getIE(),model->getIE(),eMesh,
                                      numTris,
                                      (ispc::vec3i*)index,
                                      (ispc::vec3fa*)vertex,
                                      (ispc::vec3fa*)normal,
                                      (ispc::vec4f*)color,
                                      (ispc::vec2f*)texcoord,
                                      geom_materialID,
                                      ispcMaterialPtrs,
                                      (uint32*)prim_materialID,
                                      alphaComponent,
                                      alphaType,
                                      globalAlphas,
                                      ispcMapPointers);
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

  extern "C" AlphaAwareTriangleMesh *createAlphaAwareTriangleMesh() { return new AlphaAwareTriangleMesh; }

  OSP_REGISTER_GEOMETRY(AlphaAwareTriangleMesh, alpha_aware_triangle_mesh);
}
