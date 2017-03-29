// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "TachyonSceneParser.h"
#include "Model.h"
#include "ospcommon/FileName.h"

#include <iostream>
using std::cout;
using std::endl;

using namespace ospcommon;
using namespace ospray;
using namespace ospray::tachyon;

// Helper types ///////////////////////////////////////////////////////////////

struct TimeStep
{
  std::string modelName;
  tachyon::Model tm; // tachyon model
  OSPModel       om; // ospray model

  TimeStep(const std::string &modelName) : modelName(modelName), om(nullptr) {}
};

// Helper functions ///////////////////////////////////////////////////////////

OSPModel specifyModel(tachyon::Model &tm, cpp::Renderer &renderer)
{
  OSPModel ospModel = ospNewModel();

  // TODO: This should create an array of OBJMaterials
  OSPData materialData = NULL;
  if (!tm.textureVec.empty()) {
    cout << "#osp:tachyon: specifying " << tm.numTextures() << " materials..." << endl;
    std::vector<OSPMaterial> materials;
    for (size_t i = 0; i < tm.textureVec.size(); ++i) {
      cpp::Material m = renderer.newMaterial("OBJMaterial");
      const tachyon::Texture &tex = tm.textureVec[i];
      m.set("Kd", tex.diffuse * tex.color);
      m.set("Ks", tex.phong.plastic * tex.color);
      m.set("Ns", tex.phong.size);
      m.commit();
      materials.push_back(m.handle());
    }
    materialData = ospNewData(materials.size(), OSP_OBJECT, materials.data());
    ospCommit(materialData);
  }

  if (tm.numSpheres()) {
    OSPData sphereData = ospNewData(tm.numSpheres()*sizeof(Sphere)/sizeof(float),
                                    OSP_FLOAT,tm.getSpheresPtr());
    ospCommit(sphereData);
    OSPGeometry sphereGeom = ospNewGeometry("spheres");
    ospSetData(sphereGeom,"spheres",sphereData);
    ospSet1i(sphereGeom,"bytes_per_sphere",sizeof(Sphere));
    ospSet1i(sphereGeom,"offset_materialID",0*sizeof(float));
    ospSet1i(sphereGeom,"offset_center",1*sizeof(float));
    ospSet1i(sphereGeom,"offset_radius",4*sizeof(float));
    if (materialData) {
      ospSetData(sphereGeom, "materialList", materialData);
    }
    ospCommit(sphereGeom);
    ospAddGeometry(ospModel,sphereGeom);
  }

  if (tm.numCylinders()) {
    OSPData cylinderData = ospNewData(tm.numCylinders()*sizeof(Cylinder)/sizeof(float),
                                      OSP_FLOAT,tm.getCylindersPtr());
    ospCommit(cylinderData);
    OSPGeometry cylinderGeom = ospNewGeometry("cylinders");
    ospSetData(cylinderGeom,"cylinders",cylinderData);
    ospSet1i(cylinderGeom,"bytes_per_cylinder",sizeof(Cylinder));
    ospSet1i(cylinderGeom,"offset_materialID",0*sizeof(float));
    ospSet1i(cylinderGeom,"offset_v0",1*sizeof(float));
    ospSet1i(cylinderGeom,"offset_v1",4*sizeof(float));
    ospSet1i(cylinderGeom,"offset_radius",7*sizeof(float));
    if (materialData) {
      ospSetData(cylinderGeom, "materialList", materialData);
    }
    ospCommit(cylinderGeom);
    ospAddGeometry(ospModel,cylinderGeom);
  }

  cout << "#osp:tachyon: creating " << tm.numVertexArrays() << " vertex arrays" << endl;
  long numTriangles = 0;
  for (int vaID=0;vaID<tm.numVertexArrays();vaID++) {
    const VertexArray *va = tm.getVertexArray(vaID);
    // if (va != tm.getSTriVA())
    //   continue;
    Assert(va);
    OSPGeometry geom = ospNewGeometry("trianglemesh");
    numTriangles += va->triangle.size();
    if (va->triangle.size()) {
      OSPData data = ospNewData(va->triangle.size(),OSP_INT3,&va->triangle[0]);
      ospCommit(data);
      ospSetData(geom,"triangle",data);
    }
    if (va->coord.size()) {
      OSPData data = ospNewData(va->coord.size(),OSP_FLOAT3A,&va->coord[0]);
      ospCommit(data);
      ospSetData(geom,"vertex",data);
    }
    if (va->normal.size()) {
      OSPData data = ospNewData(va->normal.size(),OSP_FLOAT3A,&va->normal[0]);
      ospCommit(data);
      ospSetData(geom,"vertex.normal",data);
    }
    if (va->color.size()) {
      OSPData data = ospNewData(va->color.size(),OSP_FLOAT4,&va->color[0]);
      ospCommit(data);
      ospSetData(geom,"vertex.color",data);
    }
    if (va->perTriTextureID.size()) {
      OSPData data = ospNewData(va->perTriTextureID.size(),OSP_UINT,
                                &va->perTriTextureID[0]);
      ospCommit(data);
      ospSetData(geom,"prim.materialID",data);
    } else {
      ospSet1i(geom,"geom.materialID",va->textureID);
    }
    ospCommit(geom);
    ospAddGeometry(ospModel,geom);
  }

  cout << "#osp:tachyon: specifying " << tm.numPointLights()
       << " point lights..." << endl;
  std::vector<OSPLight> lights;
  for (size_t i = 0; i < tm.pointLightVec.size(); ++i) {
    cpp::Light l = renderer.newLight("PointLight");
    const tachyon::PointLight &pl = tm.pointLightVec[i];
    l.set("color", pl.color);
    l.set("position", pl.center);
    l.commit();
    lights.push_back(l.handle());
  }

  cout << "#osp:tachyon: specifying " << tm.numDirLights()
       << " dir lights..." << endl;
  for (size_t i = 0; i < tm.dirLightVec.size(); ++i) {
    cpp::Light l = renderer.newLight("DirectionalLight");
    const tachyon::DirLight &dl = tm.dirLightVec[i];
    l.set("color", dl.color);
    l.set("direction", dl.direction);
    l.commit();
    lights.push_back(l.handle());
  }
  OSPData lightData = ospNewData(lights.size(), OSP_OBJECT, lights.data());
  ospCommit(lightData);
  renderer.set("lights", lightData);
  renderer.set("shadowsEnabled", 1);
  renderer.set("epsilon", 1e-4);
  renderer.commit();

  std::cout << "=======================================" << std::endl;
  std::cout << "Tachyon Renderer: Done specifying model" << std::endl;
  std::cout << "num spheres: " << tm.numSpheres() << std::endl;
  std::cout << "num cylinders: " << tm.numCylinders() << std::endl;
  std::cout << "num triangles: " << numTriangles << std::endl;
  std::cout << "=======================================" << std::endl;


  ospCommit(ospModel);
  return ospModel;
}

// Class definitions //////////////////////////////////////////////////////////

TachyonSceneParser::TachyonSceneParser(cpp::Renderer renderer) :
  renderer(renderer)
{
}

bool TachyonSceneParser::parse(int ac, const char **&av)
{
  bool loadedScene = false;

  for (int i = 1; i < ac; i++) {
    std::string arg = av[i];
    if (arg[0] != '-') {
      FileName fn = arg;
      if (fn.ext() == "tachy") {
        loadedScene = true;
        TimeStep ts(arg);
        importFile(ts.tm, arg);
        ts.om = specifyModel(ts.tm, renderer);
        sceneModels.push_back(ts.om);
        sceneBboxs.push_back(ts.tm.getBounds());
      }
    }
  }

  return loadedScene;
}

std::deque<cpp::Model> TachyonSceneParser::model() const
{
  return sceneModels;
}

std::deque<box3f> TachyonSceneParser::bbox() const
{
  return sceneBboxs;
}
