/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

#include "sg/geometry/Spheres.h"

namespace ospray {
  namespace sg {


    // =======================================================
    /*! \brief Realizes a set of spheres with associated attributes
        and transfer function for color- and alaha-mapping.  */
    // =======================================================
    struct PKDGeometry : public sg::Geometry {

      struct Particle {
        vec3f position;
        float attribute;
      };

      //! constructor
      PKDGeometry() 
        : Geometry("pkd_geometry"), 
          transferFunction(NULL),
          ospParticleData(NULL),
          ospGeometry(NULL)
      {};

      //! destructor
      virtual ~PKDGeometry() { throw std::runtime_error("pkd-spheres destructed!?"); }
      
      //! set radius to use for the spheres
      void setRadius(const float radius);

      //! set transferFunction to use for the spheres
      void setTransferFunction(Ref<sg::TransferFunction> transferFunction);

      void setParticles(const std::vector<Particle> &particles);
      void loadParticles(const std::string &fileName);

      //! \brief Initialize this node's value from given XML node 
      virtual void setFromXML(const xml::Node *const node);
      

      //! return bounding box of this node (in local space)
      virtual box3f getBounds();

      /*! 'render' the nodes - all geometries, materials, etc will
          create their ospray counterparts, and store them in the
          node  */
      virtual void render(World *world=NULL, 
                          Integrator *integrator=NULL,
                          const affine3f &xfm = embree::one);

    protected:
      /*! single radius applied to all spheres in this geometry */
      float radius;
      
      //! the ospray geometry we use for this set of alpha-spheres
      OSPGeometry ospGeometry;

      //! vector os sphere particles; one per sphere
      std::vector<Particle> particle;

      //! data array that keeps the sphere particles
      OSPData ospParticleData;

      //! the transfer function we use for color-mapping the given attribute
      Ref<TransferFunction>           transferFunction;
      
    public:
      static sg::World *importPKDFile(const std::string &fileName);
    };
    
  } // ::ospray::sg
} // ::ospray


