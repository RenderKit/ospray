// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

#include "ParticleSceneParser.h"

// particle viewer
#include "Model.h"
#include "uintah.h"

#include <random>

#include <ospray_cpp/Data.h>

using namespace ospray;
using namespace ospcommon;

// TODO: Need to convert ospray.h calls to ospray::cpp objects!

// Helper functions ///////////////////////////////////////////////////////////

particle::Model *createTestCube(int numPerSide)
{
  particle::Model *m = new particle::Model;
  int type = m->getAtomType("testParticle");
  for (int z=0;z<numPerSide;z++)
    for (int y=0;y<numPerSide;y++)
      for (int x=0;x<numPerSide;x++) {
        particle::Model::Atom a;
        a.position.x = x/float(numPerSide);
        a.position.y = y/float(numPerSide);
        a.position.z = z/float(numPerSide);
        a.type = type;
        a.radius = 1.f/numPerSide;
        m->atom.push_back(a);
      }
  return m;
}

OSPData makeMaterials(OSPRenderer renderer, particle::Model *model)
{
  int numMaterials = model->atomType.size();
  OSPMaterial matArray[numMaterials];
  for (int i = 0; i < numMaterials; i++) {
    OSPMaterial mat = ospNewMaterial(renderer,"OBJMaterial");
    assert(mat);
    ospSet3fv(mat,"kd",&model->atomType[i]->color.x);
    ospCommit(mat);
    matArray[i] = mat;
  }
  OSPData data = ospNewData(numMaterials,OSP_OBJECT,matArray);
  ospCommit(data);
  return data;
}

// Helper types ///////////////////////////////////////////////////////////////

struct DeferredLoadJob {
  DeferredLoadJob(particle::Model *model,
                  const FileName &xyzFileName,
                  const FileName &defFileName)
    : model(model), xyzFileName(xyzFileName), defFileName(defFileName)
  {}

  //! the mode we still have to load
  particle::Model *model;
  //! file name of xyz file to be loaded into this model
  FileName xyzFileName;
  //! name of atom type defintion file active when this xyz file was added
  FileName defFileName;
};

// Class definitions //////////////////////////////////////////////////////////

ParticleSceneParser::ParticleSceneParser(cpp::Renderer renderer) :
  m_renderer(renderer)
{
}

bool ParticleSceneParser::parse(int ac, const char **&av)
{
  bool loadedScene = false;
#if 1
  std::vector<particle::Model *> particleModel;
  std::vector<DeferredLoadJob *> deferredLoadingListXYZ;
  FileName defFileName = "";
  std::vector<OSPModel> modelTimeStep;
  int timeStep = 0;

  for (int i = 1; i < ac; i++) {
    const std::string arg = av[i];
    if (arg == "--radius") {
      ospray::particle::Model::defaultRadius = atof(av[++i]);
    } else if (arg == "--atom-defs") {
      defFileName = av[++i];
    } else if (arg == "--particle-timestep") {
      timeStep = atoi(av[++i]);
    } else {
      FileName fn = arg;
      if (fn.str() == "___CUBE_TEST___") {
        int numPerSide = atoi(av[++i]);
        particle::Model *m = createTestCube(numPerSide);
        particleModel.push_back(m);
        loadedScene = true;
      } else if (fn.ext() == "xyz") {
        particle::Model *m = new particle::Model;
        deferredLoadingListXYZ.push_back(new DeferredLoadJob(m,fn,defFileName));
        particleModel.push_back(m);
        loadedScene = true;
      } else if (fn.ext() == "xyz2") {
        particle::Model *m = new particle::Model;
        m->loadXYZ2(fn);
        particleModel.push_back(m);
        loadedScene = true;
#if 1 // NOTE(jda) - The '.xml' file extension conflicts with RIVL files in
      //             TriangleMeshSceneParser...disabling here for now until the
      //             the problem requires a solution.
      }
#else
      } else if (fn.ext() == "xml") {
        particle::Model *m = particle::parse__Uintah_timestep_xml(fn);
        particleModel.push_back(m);
        loadedScene = true;
      }
#endif
    }
  }

  if (loadedScene) {
    //TODO: this needs parallelized as it was in ospParticleViewer...
    for (int i = 0; i < deferredLoadingListXYZ.size(); ++i) {
      FileName defFileName = deferredLoadingListXYZ[i]->defFileName;
      FileName xyzFileName = deferredLoadingListXYZ[i]->xyzFileName;
      particle::Model *model = deferredLoadingListXYZ[i]->model;

      if (defFileName.str() != "")
        model->readAtomTypeDefinitions(defFileName);
      model->loadXYZ(xyzFileName);
    }

    for (int i = 0; i < particleModel.size(); i++) {
      OSPModel model = ospNewModel();
      OSPData materialData = makeMaterials(m_renderer.handle(), particleModel[i]);

      OSPData data = ospNewData(particleModel[i]->atom.size()*5,OSP_FLOAT,
                                &particleModel[i]->atom[0],OSP_DATA_SHARED_BUFFER);
      ospCommit(data);

      OSPGeometry geom = ospNewGeometry("spheres");
      ospSet1i(geom,"bytes_per_sphere",sizeof(particle::Model::Atom));
      ospSet1i(geom,"offset_center",0);
      ospSet1i(geom,"offset_radius",3*sizeof(float));
      ospSet1i(geom,"offset_materialID",4*sizeof(float));
      ospSetData(geom,"spheres",data);
      ospSetData(geom,"materialList",materialData);
      ospCommit(geom);

      ospAddGeometry(model,geom);
      ospCommit(model);

      modelTimeStep.push_back(model);
    }

    m_model = modelTimeStep[timeStep];
    m_bbox  = particleModel[0]->getBBox();
  }

#elif 1
  createSpheres();
  loadedScene = true;
#else
  createCylinders();
  loadedScene = true;
#endif
  return loadedScene;
}

ospray::cpp::Model ParticleSceneParser::model() const
{
  return m_model;
}

ospcommon::box3f ParticleSceneParser::bbox() const
{
  return m_bbox;
}

void ParticleSceneParser::createSpheres()
{
  struct Sphere {
    vec3f center;
    uint  colorID;
  };

  std::vector<Sphere> spheres;
  std::vector<vec4f>  colors;

#define NUM_SPHERES 100000
#define NUM_COLORS 10

  spheres.resize(NUM_SPHERES);
  colors.resize(NUM_SPHERES);

  std::default_random_engine rng;
  std::uniform_real_distribution<float> vdist(-1000.0f, 1000.0f);
  std::uniform_real_distribution<float> cdist(0.0f, 1.0f);
  std::uniform_int_distribution<uint>   ciddist(0, NUM_COLORS-1);

  for (int i = 0; i < NUM_SPHERES; i++) {
    spheres[i].center.x = vdist(rng);
    spheres[i].center.y = vdist(rng);
    spheres[i].center.z = vdist(rng);
    spheres[i].colorID  = ciddist(rng);
  }

  for (int i = 0; i < NUM_COLORS; i++) {
    colors[i].x = cdist(rng);
    colors[i].y = cdist(rng);
    colors[i].z = cdist(rng);
    colors[i].w = 1.0f;
  }

  auto sphereData = cpp::Data(sizeof(Sphere)*NUM_SPHERES, OSP_CHAR,
                              spheres.data());
  auto colorData  = cpp::Data(NUM_COLORS, OSP_FLOAT4, colors.data());

  sphereData.commit();
  colorData.commit();

  auto geometry = cpp::Geometry("spheres");
  geometry.set("spheres", sphereData);
  geometry.set("color",   colorData);
  geometry.set("radius", 10.f);
  geometry.set("bytes_per_sphere", int(sizeof(Sphere)));
  geometry.set("offset_colorID", int(sizeof(vec3f)));
  geometry.commit();

  m_model.addGeometry(geometry);
  m_model.commit();
}

void ParticleSceneParser::createCylinders()
{
  struct Cylinder {
    vec3f v0;
    vec3f v1;
    uint  colorID;
  };

  std::vector<Cylinder> cylinders;
  std::vector<vec4f>  colors;

#define NUM_CYLINDERS 100
#define NUM_COLORS 10

  cylinders.resize(NUM_CYLINDERS);
  colors.resize(NUM_CYLINDERS);

  std::default_random_engine rng;
  std::uniform_real_distribution<float> vdist(-1000.0f, 1000.0f);
  std::uniform_real_distribution<float> cdist(0.0f, 1.0f);
  std::uniform_int_distribution<uint>   ciddist(0, NUM_COLORS-1);

  for (int i = 0; i < NUM_CYLINDERS; i++) {
    cylinders[i].v0.x = vdist(rng);
    cylinders[i].v0.y = vdist(rng);
    cylinders[i].v0.z = vdist(rng);
    cylinders[i].v1.x = vdist(rng);
    cylinders[i].v1.y = vdist(rng);
    cylinders[i].v1.z = vdist(rng);
    cylinders[i].colorID  = ciddist(rng);
  }

  for (int i = 0; i < NUM_COLORS; i++) {
    colors[i].x = cdist(rng);
    colors[i].y = cdist(rng);
    colors[i].z = cdist(rng);
    colors[i].w = 1.0f;
  }

  auto cylinderData = cpp::Data(sizeof(Cylinder)*NUM_CYLINDERS, OSP_CHAR,
                                cylinders.data());
  auto colorData  = cpp::Data(NUM_COLORS, OSP_FLOAT4, colors.data());

  cylinderData.commit();
  colorData.commit();

  auto geometry = cpp::Geometry("cylinders");
  geometry.set("cylinders", cylinderData);
  geometry.set("color",   colorData);
  geometry.set("radius", 10.f);
  geometry.set("bytes_per_cylinder", int(sizeof(Cylinder)));
  geometry.set("offset_colorID", int(2*sizeof(vec3f)));
  geometry.commit();

  m_model.addGeometry(geometry);
  m_model.commit();
}
