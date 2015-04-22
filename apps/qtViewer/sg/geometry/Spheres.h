// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
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

#pragma once

#include "sg/common/Node.h"
#include "sg/common/TransferFunction.h"
#include "sg/geometry/Geometry.h"

namespace ospray {
  namespace sg {

    /*! simple spheres, with all of the key info - position, radius,
        and a int32 type specifier baked into each sphere  */
    struct Spheres : public sg::Geometry {
      struct Sphere { 
        vec3f position;
        float radius;
        uint32 typeID;
        
        // constructor
        Sphere(vec3f position, float radius, uint typeID=0);
        
        // return the bounding box
        inline box3f getBounds() const
        { return box3f(position-vec3f(radius),position+vec3f(radius)); };
      };

      //! constructor
      Spheres() : Geometry("spheres"), ospGeometry(NULL) {};
      
      // return bounding box of all primitives
      virtual box3f getBounds();

      /*! 'render' the nodes */
      virtual void render(RenderContext &ctx);

      OSPGeometry         ospGeometry;
      std::vector<Sphere> sphere;
    };


    // // =======================================================
    // /*! \brief Realizes a set of spheres with associated attributes
    //     and transfer function for color- and alaha-mapping.  */
    // // =======================================================
    // struct AlphaSpheres : public sg::Geometry {

    //   //! constructor
    //   AlphaSpheres() 
    //     : Geometry("alpha_spheres"), 
    //       transferFunction(NULL),
    //       ospPositionData(NULL),
    //       ospGeometry(NULL)
    //   {};

    //   //! destructor
    //   virtual ~AlphaSpheres() { throw std::runtime_error("alpha-spheres destructed!?"); }
      
    //   //! set of attributes. must have one attribute value per valid
    //   //! ID in the spheres
    //   struct Attribute : public embree::RefCount, public std::vector<float> {

    //     //! constructor
    //     Attribute(const std::string &name) : name(name), ospData(NULL) {};

    //   protected:
    //     const std::string name;
    //     //! the data we use to store this attribute in
    //     OSPData ospData;

    //     friend class AlphaSpheres;
    //   };

    //   //! set radius to use for the spheres
    //   void setRadius(const float radius);

    //   //! set transferFunction to use for the spheres
    //   void setTransferFunction(Ref<sg::TransferFunction> transferFunction);

    //   //! \brief set radius to use for the spheres
    //   /*! \detailed This function may NOT get called once the geometry
    //       got 'render'ed for the first time */
    //   void setPositions(const std::vector<vec3f> &positions);

    //   //! return a list of the names of all attributes we have in this geometry
    //   std::vector<std::string> getAttributeNames() const;

    //   //! add a new set of attributes to the geometry
    //   void addAttribute(Attribute *attribute);

    //   //! add a new set of attributes to the geometry
    //   void setActiveAttribute(Attribute *attribute);

    //   //! select a new attribute
    //   void setActiveAttribute(const std::string &attrName) 
    //   { setActiveAttribute(attribute[attrName].ptr); }


    //   //! \brief Initialize this node's value from given XML node 
    //   virtual void setFromXML(const xml::Node *const node);
      
    //   //! return bounding box of this node (in local space)
    //   virtual box3f getBounds();

    //   /*! 'render' the nodes - all geometries, materials, etc will
    //       create their ospray counterparts, and store them in the
    //       node  */
    //   virtual void render(RenderContext &ctx);

    // protected:
    //   /*! single radius applied to all spheres in this geometry */
    //   float radius;
      
    //   //! the ospray geometry we use for this set of alpha-spheres
    //   OSPGeometry ospGeometry;

    //   //! vector os sphere positions; one per sphere
    //   std::vector<vec3f> position;

    //   //! data array that keeps the sphere positions
    //   OSPData ospPositionData;

    //   //! the different sets of attributes that can be used for this sphere
    //   std::map<std::string,Ref<Attribute> > attribute;

    //   //! the attribute currently used for color mapping
    //   Ref<Attribute> activeAttribute;

    //   //! the transfer function we use for color-mapping the given attribute
    //   Ref<TransferFunction>           transferFunction;
      
    // public:
    //   static sg::World *importOspAtomFile(const std::string &fileName);
    // };
    
  } // ::ospray::sg
} // ::ospray


