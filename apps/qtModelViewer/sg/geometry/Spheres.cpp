/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

#undef NDEBUG

#include "sg/geometry/Spheres.h"
#include "sg/common/Integrator.h"
// xml parser
#include "apps/common/xml/xml.h"

namespace ospray {
  namespace sg {

    using std::string;
    using std::cout;
    using std::endl;

    //! return bounding box of this node (in local space)
    box3f AlphaSpheres::getBounds() 
    {
      box3f bounds = embree::empty;
      for (size_t i=0;i<position.size();i++)
        bounds.extend(position[i]);
      bounds.lower -= vec3f(radius);
      bounds.upper += vec3f(radius);
      return bounds;
    }

    void AlphaSpheres::render(World *world, 
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
        ospGeometry = ospNewGeometry("alpha_spheres");
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
      
      // create the position array
      if (!ospPositionData) {
        if (position.empty())
          throw std::runtime_error("osp:sg:AlphaSpheres: no 'position's defined");
        ospPositionData = ospNewData(position.size(),OSP_FLOAT3,&position[0],
                                     OSP_DATA_SHARED_BUFFER);
        ospCommit(ospPositionData);
        ospSetData(ospGeometry,"positions",ospPositionData);
      }
      
      // check if transfer function exists, and is updated
      PING;
      PRINT(transferFunction);
      if (transferFunction == NULL)
        throw std::runtime_error("osp:sg:AlphaSpheres: no 'transferFunction' defined");
      else {
        transferFunction->render(world,integrator,_xfm);
        if (transferFunction->getLastCommitted() >= this->lastCommitted) 
          ospSetObject(ospGeometry,"transferFunction",transferFunction->getOSPHandle());
      }

      // make sure active attribute is presetnt ...
      PING;
      PRINT(activeAttribute);
      if (activeAttribute == NULL)
        throw std::runtime_error("osp:sg:AlphaSpheres: no 'activeAttribute' defined");
      // ... and commited ...
      if (activeAttribute->ospData == NULL) {
        assert(activeAttribute->size() == position.size());
        activeAttribute->ospData = ospNewData(activeAttribute->size(),OSP_FLOAT,
                                              &*activeAttribute->begin(),
                                              OSP_DATA_SHARED_BUFFER);        
        ospCommit(activeAttribute->ospData);
      }
      // ... ; then assign this attribute to the geometry
      ospSetData(ospGeometry,"attributes",activeAttribute->ospData);

      assert(radius > 0.f);
      ospSet1f(ospGeometry,"radius",radius);
      
      ospCommit(ospGeometry);
      lastCommitted = TimeStamp::now();
    }


    //! \brief Initialize this node's value from given corresponding XML node 
    void AlphaSpheres::setFromXML(const xml::Node *const node)
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
          std::cout << "#osp:sg:AlphaSpheres: Warning - unknown child field type '" << child->name << "'" << std::endl;
      }

      if (!transferFunction) {
        std::cout << "#osp:sg:AlphaSpheres: Warning - no transfer function specified" << std::endl;
        transferFunction = new TransferFunction();
      }
    }

      //! return a list of the names of all attributes we have in this geometry
    std::vector<std::string> AlphaSpheres::getListOfAttributes() const
    {
      std::vector<std::string> names;
      for (std::map<std::string,Ref<Attribute> >::const_iterator it = attribute.begin(); 
           it != attribute.end(); ++it)
        names.push_back(it->first);
      return names;
    }


    //! set radius to use for the spheres
    void AlphaSpheres::setRadius(const float radius)
    { 
      this->radius = radius; 
      lastModified = TimeStamp::now(); 
    }

    //! set transferFunction to use for the spheres
    void AlphaSpheres::setTransferFunction(Ref<sg::TransferFunction> transferFunction)
    { 
      this->transferFunction = transferFunction; 
      lastModified = TimeStamp::now(); 
    }

    //! set radius to use for the spheres
    void AlphaSpheres::setPositions(const std::vector<vec3f> &positions)
    {
      this->position = positions; 
      if (ospPositionData) { ospRelease(ospPositionData); ospPositionData = NULL; }
      lastModified = TimeStamp::now(); 
    }

    //! add a new set of attributes to the geometry
    void AlphaSpheres::setActiveAttribute(Attribute *attribute)
    {
      this->activeAttribute = attribute; 
      lastModified = TimeStamp::now(); 
    }

    //! add a new set of attributes to the geometry
    void AlphaSpheres::addAttribute(Attribute *attribute)
    {
      assert(attribute != NULL);
      this->attribute[attribute->name] = attribute;
      PRINT(this->attribute.size());
      if (this->attribute.size() == 1)
        setActiveAttribute(attribute);
    }

    OSP_REGISTER_SG_NODE(AlphaSpheres)

  }
}
