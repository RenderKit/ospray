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

#pragma once

#include "sg/common/Common.h"
// ospcommon
#include "ospcommon/AffineSpace.h"
// std
#include <vector>

namespace ospray {
  namespace sg {

    /*! class one can use to serialize all nodes in the scene graph */
    struct OSPSG_INTERFACE Serialization
    {
      enum Mode
      {
        /*! when serializing the scene graph, traverse through all
         instances and record each and every occurrence of any object
         (ie, an instanced object will appear multiple times in the
         output, each with one instantiation info */
        DO_FOLLOW_INSTANCES, 
        /*! when serializing the scene graph, record each instanced
            object only ONCE, and list all its instantiations in its
            instantiation vector */
        DONT_FOLLOW_INSTANCES
      };
      
      struct OSPSG_INTERFACE Instantiation
      {
        std::shared_ptr<Instantiation> parentWorld;
        affine3f xfm {one};
      };
      
      /*! describes one object that we encountered */
      struct OSPSG_INTERFACE Object
      {
        /*! the node itself - this is intentionally NOT a shared_ptr
            since nodes call this with 'this', for which we have no
            shared_ptr info */
        std::shared_ptr<sg::Node> node;  

        /*! the instantiation info when we traversed this node. May be
          NULL if object isn't instanced (or only instanced once) */
        std::shared_ptr<Instantiation> instantiation;

        Object(const std::shared_ptr<sg::Node> &node,
               std::shared_ptr<Instantiation> inst)
          : node(node), instantiation(inst)  {}
      };

      /*! the node that maintains all the traversal state when
          traversing the scene graph */
      struct OSPSG_INTERFACE State
      {
        std::shared_ptr<Instantiation> instantiation;
        Serialization *serialization;
      };

      void serialize(std::shared_ptr<sg::World> world,
                     Serialization::Mode mode);
      
      /*! clear all old objects */
      void clear() { object.clear(); }

      size_t size() const { return object.size(); }

      /*! the vector containing all the objects encountered when
          serializing the entire scene graph */
      std::vector<std::shared_ptr<Serialization::Object> > object;
    };
    
  } // ::ospray::sg
} // ::ospray


