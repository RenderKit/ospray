/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

#include "sg/common/Common.h"

namespace ospray {
  namespace sg {
    
    /*! class one can use to serialize all nodes in the scene graph */
    struct Serialization {
      typedef enum { 
        /*! when serializing the scene graph, traverse through all
         instances and record each and every occurrence of any object
         (ie, an instanced object will appear multiple times in the
         output, each with one instantiation info */
        DO_FOLLOW_INSTANCES, 
        /*! when serializing the scene graph, record each instanced
            object only ONCE, and list all its instantiations in its
            instantiation vector */
        DONT_FOLLOW_INSTANCES
      } Mode;
      struct Instantiation : public embree::RefCount {
        Ref<Instantiation> parentWorld;
        affine3f           xfm;

        Instantiation() : parentWorld(NULL), xfm(embree::one) {}
      };
      /*! describes one object that we encountered */
      struct Object : public embree::RefCount {
        /*! the node itself */
        Ref<sg::Node>      node;  

        /*! the instantiation info when we traversed this node. May be
          NULL if object isn't instanced (or only instanced once) */
        Ref<Instantiation> instantiation;
        // std::vector<Ref<Instantiation> > instantiation;

        Object(sg::Node *node=NULL, Instantiation *inst=NULL)
          : node(node), instantiation(inst) 
        {};
      };

      /*! the node that maintains all the traversal state when
          traversing the scene graph */
      struct State {
        Ref<Instantiation> instantiation;
        Serialization *serialization;
      };

      void serialize(Ref<sg::World> world, Serialization::Mode mode);
      
      /*! clear all old objects */
      void clear() {  object.clear(); }

      size_t size() const { return object.size(); }

      /*! the vector containing all the objects encountered when
          serializing the entire scene graph */
      std::vector<Ref<Object> > object;
    };
    

  } // ::ospray::sg
} // ::ospray


