/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#undef NDEBUG

#include "sg/geometry/PKD.h"
#include "sg/common/Integrator.h"
// xml parser
#include "apps/common/xml/xml.h"

namespace ospray {
  namespace sg {

    using std::string;
    using std::cout;
    using std::endl;

    //! return bounding box of this node (in local space)
    box3f PKDGeometry::getBounds() 
    {
      box3f bounds = embree::empty;
      for (size_t i=0;i<particle.size();i++)
        bounds.extend(particle[i].position);
      bounds.lower -= vec3f(radius);
      bounds.upper += vec3f(radius);
      return bounds;
    }

    void PKDGeometry::render(World *world, 
                              Integrator *integrator,
                              const affine3f &_xfm)
    {
      assert(!ospGeometry);
      
      // check if the data has changed at all
      if (lastModified <= lastCommitted) 
        // ... and return if not
        return;

      // if no geometry exists, create it.
      if (!ospGeometry) {
        // make sure the module is loaded
        ospLoadModule("alpha_spheres");

        // and create the geometry
        ospGeometry = ospNewGeometry("pkd_geometry");
        assert(ospGeometry);

        // assign a default material (for now.... eventaully we might
        // want to do a 'real' mateiral
        OSPMaterial mat = ospNewMaterial(integrator?integrator->getOSPHandle():NULL,"default");
        if (mat) {
          vec3f kd = .7f;
          ospSet3fv(mat,"kd",&kd.x);
        }
        ospSetMaterial(ospGeometry,mat);

        // and finally, add this geometry to the model
        ospAddGeometry(world->ospModel,ospGeometry);
      }

      // create the particle array
      if (!ospParticleData) {
        if (particle.empty())
          throw std::runtime_error("osp:sg:PKDGeometry: no 'particle's defined");
        ospParticleData = ospNewData(particle.size(),OSP_FLOAT4,&particle[0],
                                     OSP_DATA_SHARED_BUFFER);
        ospCommit(ospParticleData);
        ospSetData(ospGeometry,"particles",ospParticleData);
      }
      
      // check if transfer function exists, and is updated
      if (transferFunction == NULL)
        throw std::runtime_error("osp:sg:PKDGeometry: no 'transferFunction' defined");
      else {
        transferFunction->render(world,integrator,_xfm);
        if (transferFunction->getLastCommitted() >= this->lastCommitted) 
          ospSetObject(ospGeometry,"transferFunction",transferFunction->getOSPHandle());
      }
      
      assert(radius > 0.f);
      ospSet1f(ospGeometry,"radius",radius);
      
      ospCommit(ospGeometry);
      lastCommitted = TimeStamp::now();
    }


    //! \brief Initialize this node's value from given corresponding XML node 
    void PKDGeometry::setFromXML(const xml::Node *const node)
    {
      for (size_t childID=0;childID<node->child.size();childID++) {
        xml::Node *child = node->child[childID];
        if (child->name == "transferFunction") {
          if (child->getProp("ref") != "")
            transferFunction = dynamic_cast<sg::TransferFunction*>(findNamedNode(child->getProp("ref")));
          else if (child->child.size()) {
            Ref<sg::Node> n = sg::parseNode(child->child[0]);
            transferFunction = n.cast<sg::TransferFunction>();
          }
        }
        else
          std::cout << "#osp:sg:PKDGeometry: Warning - unknown child field type '" << child->name << "'" << std::endl;
      }

      if (!transferFunction) {
        std::cout << "#osp:sg:PKDGeometry: Warning - no transfer function specified" << std::endl;
        transferFunction = new TransferFunction();
      }
    }


    //! set radius to use for the spheres
    void PKDGeometry::setRadius(const float radius)
    { 
      this->radius = radius; 
      lastModified = TimeStamp::now(); 
    }

    //! set transferFunction to use for the spheres
    void PKDGeometry::setTransferFunction(Ref<sg::TransferFunction> transferFunction)
    { 
      this->transferFunction = transferFunction; 
      lastModified = TimeStamp::now(); 
    }

    //! set radius to use for the spheres
    void PKDGeometry::setParticles(const std::vector<Particle> &particles)
    {
      this->particle = particles; 
      if (ospParticleData) { ospRelease(ospParticleData); ospParticleData = NULL; }
      lastModified = TimeStamp::now(); 
    }

    sg::World *PKDGeometry::importPKDFile(const std::string &fileName)
    {
      FILE *file = fopen(fileName.c_str(),"r");
      fseek(file,0,SEEK_END);
      size_t numParticles = ftell(file) / sizeof(Particle);
      fseek(file,0,SEEK_SET);

      char *maxAtomsEnv = getenv("OSPRAY_MAX_ATOMS");
      if (maxAtomsEnv) 
        numParticles = std::min(numParticles,(size_t)atol(maxAtomsEnv));

      Ref<sg::PKDGeometry> pkd = new sg::PKDGeometry;
      Ref<sg::TransferFunction> transferFunction = new sg::TransferFunction;
      pkd->setTransferFunction(transferFunction);
      pkd->setRadius(2e-3f);
      
      pkd->particle.resize(numParticles);
      for (int i=0;i<numParticles;i++) {
        fread(&pkd->particle[i],sizeof(Particle),1,file);
      }
      fclose(file);
      
      sg::World *world = new sg::World;
      world->node.push_back(transferFunction.ptr);
      world->node.push_back(pkd.ptr);

      return world;
    }

    OSP_REGISTER_SG_NODE(PKDGeometry)

  }
}
