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

#include "sg/geometry/Spheres.h"

namespace ospray {
  namespace sg {

    // =======================================================
    /*! \brief Realizes a set of spheres with associated attributes
        and transfer function for color- and alaha-mapping.  */
    // =======================================================
    struct PKDGeometry : public sg::Geometry {

      //! constructor
      PKDGeometry();

      //! destructor
      virtual ~PKDGeometry();
      
      //! set radius to use for the spheres
      void setRadius(const float radius);

      //! set transferFunction to use for the spheres
      void setTransferFunction(Ref<sg::TransferFunction> transferFunction);

      // void setParticles(const std::vector<Particle> &particles);
      // void loadParticles(const std::string &fileName);

      //! \brief Initialize this node's value from given XML node 
      virtual void setFromXML(const xml::Node *const node, const unsigned char *binBasePtr);
      

      //! return bounding box of this node (in local space)
      virtual box3f getBounds();

      /*! 'render' the nodes - all geometries, materials, etc will
          create their ospray counterparts, and store them in the
          node  */
      virtual void render(RenderContext &ctx);


      //! serialize into given serialization state 
      virtual void serialize(sg::Serialization::State &state)
      {
        Geometry::serialize(state);
        if (transferFunction) 
          transferFunction->serialize(state);
      }

      vec3f getParticle(size_t i) const;

      
    protected:
      struct Attribute {
        std::string name;
        float      *value;
        float       minValue, maxValue;
        OSPData     ospData;

        Attribute() : ospData(NULL) {};
      };

      /*! number of particles */
      size_t numParticles; 

      /*! array of particle positions, in kd-tree order */
      union {
        vec3f *particle3f; 
        uint64 *particle1ul;
      };
      OSPDataType format;

      /*! particle bounding box */
      box3f particleBounds;

      /*! list of attributes; each attribute must have numParticles values */
      std::vector<Attribute *> attribute;

      /*! single radius applied to all spheres in this geometry */
      float radius;
      
      //! the ospray geometry we use for this set of alpha-spheres
      OSPGeometry ospGeometry;

      //! data array that keeps the sphere particles
      OSPData ospPositionData;

      //! the transfer function we use for color-mapping the given attribute
      Ref<TransferFunction>           transferFunction;
      
      /*! if enabled, we'll use the old ospray alpha-spheres code with
          min-max BVH rather than a pkd-tree. by default this should
          be 'false', but by adding a "<useOldAlphaSpheresCode value='1'/>"
          statement to the xml node we can enable the old code for
          comparison purposes */
      bool useOldAlphaSpheresCode;
    // public:
    //   static sg::World *importPKDFile(const std::string &fileName);
    };
    
  } // ::ospray::sg
} // ::ospray


