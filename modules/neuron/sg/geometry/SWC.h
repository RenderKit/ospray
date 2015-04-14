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

/*! \file importer/SVC.cpp Scene Graph Importer Plugin for Neuromorpho SWC files */

// ospray
// ospray/sg
#include "sg/geometry/Geometry.h"
	 
namespace ospray {
  namespace sg {
    
    struct SWCGeometry : public sg::Geometry {
      struct Node {
        //! constructor
        Node() {};
        //! constructor
        Node(const vec3f &pos, const float rad) : pos(pos), rad(rad) {};
        //! constructor
        Node(const Node &other) : pos(other.pos), rad(other.rad) {};

        //! (center) position of node/control point
        vec3f pos;

        //! radius at this node
        float rad;
      };
      //! vector of input nodes (represented via spheres)
      std::vector<Node>                nodeVec;
      //! vector of input links(represented via cones)
      std::vector<std::pair<int,int> > linkVec;
      //! set if we want ospray to double-check the geometry for duplicates etc
      int checkData;

      /*! vector that conatins a vector of "first node index, first
          link index" vector that denotes where different *compound
          objects* (such as a neuron, or a stream line) start (they
          point into the nodeVec/linkVec's) */
      std::vector<std::pair<int,int> > startIndexVec;

      SWCGeometry();
      
      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const
      { return "ospray::sg::SWCGeometry (SWC module)"; }

      /*! 'render' the nodes - all geometries, materials, etc will
          create their ospray counterparts, and store them in the
          node  */
      virtual void render(RenderContext &ctx);

      //! return bounding box of all primitives
      virtual box3f getBounds();

      //! all the comments found in the original SWC file
      std::string originalDescription;

      OSPGeometry ospGeometry;
      OSPData     ospNodeData;
      OSPData     ospLinkData;
    };

  }
}

