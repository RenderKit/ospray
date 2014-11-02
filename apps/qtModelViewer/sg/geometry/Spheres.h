/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

#include "sg/common/Node.h"

namespace ospray {
  namespace sg {

    /*! simple spheres, with all of the key info - position, radius,
        and a int32 type specifier baked into each sphere  */
    struct Spheres : public sg::Geometry {
      struct Sphere { 
        vec3f position;
        float radius;
        uint32 typeID;
        
        Sphere(vec3f position, 
               float radius,
               uint typeID=0) 
          : position(position), 
            radius(radius), 
            typeID(typeID) 
        {};

        inline box3f getBounds() const
        { return box3f(position-vec3f(radius),position+vec3f(radius)); };

      };

      OSPGeometry ospGeometry;
      /*! one material per typeID */
      // std::vector<Ref<sg::Material> > material;
      std::vector<Sphere>             sphere;

      Spheres() : Geometry("spheres"), ospGeometry(NULL) {};

      virtual box3f getBounds() {
        box3f bounds = embree::empty;
        for (size_t i=0;i<sphere.size();i++)
          bounds.extend(sphere[i].getBounds());
        return bounds;
      }
      /*! 'render' the nodes - all geometries, materials, etc will
          create their ospray counterparts, and store them in the
          node  */
      virtual void render(World *world=NULL, 
                          Integrator *integrator=NULL,
                          const affine3f &xfm = embree::one);
      
      
    };


    /*! simple spheres, with all of the key info - position, radius,
        and a int32 type specifier baked into each sphere  */
    struct AlphaSpheres : public sg::Geometry {
      //! note: currently MUST be a multiple 8 b in size
      struct Sphere { 
        vec3f position;
        float radius;
        float attribute;
        int typeID;
        
        Sphere(vec3f position, 
               float radius,
               float attribute,
               int typeID=0) 
          : position(position), 
            radius(radius), 
            attribute(attribute),
            typeID(typeID)
        {};

        inline box3f getBounds() const
        { return box3f(position-vec3f(radius),position+vec3f(radius)); };

      };

      OSPGeometry ospGeometry;
      /*! one material per typeID */
      // std::vector<Ref<sg::Material> > material;
      std::vector<Sphere>             sphere;
      Ref<TransferFunction>           transferFunction;


      AlphaSpheres() 
        : Geometry("alpha_spheres"), 
          ospGeometry(NULL), 
          transferFunction(new TransferFunction) 
      {};
      virtual ~AlphaSpheres() { throw std::runtime_error("alpha-spheres destructed!?"); }
      virtual void setFromXML(const xml::Node *const node);

      virtual box3f getBounds() {
        box3f bounds = embree::empty;
        for (size_t i=0;i<sphere.size();i++)
          bounds.extend(sphere[i].getBounds());
        return bounds;
      }
      /*! 'render' the nodes - all geometries, materials, etc will
          create their ospray counterparts, and store them in the
          node  */
      virtual void render(World *world=NULL, 
                          Integrator *integrator=NULL,
                          const affine3f &xfm = embree::one);
    };
    
  } // ::ospray::sg
} // ::ospray


