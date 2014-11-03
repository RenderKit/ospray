/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

#include "sg/common/Node.h"
#include "sg/common/Serialization.h"
#include "sg/camera/Camera.h"

namespace ospray {
  namespace sg {

    /*! a world node */
    struct World : public sg::Node {
      World() : ospModel(NULL) {};

      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const { return "ospray::viewer::sg::World"; }

      //! serialize into given serialization state 
      virtual void serialize(sg::Serialization::State &serialization);

      std::vector<Ref<Node> > node;

      OSPModel ospModel;
      virtual void render(World *world=NULL, 
                          Integrator *integrator=NULL,
                          const affine3f &xfm = embree::one);

      /*! \brief return bounding box in world coordinates.

        This function can be used by the viewer(s) for calibrating
        camera motion, setting default camera position, etc. Nodes
        for which that does not apply can simpy return
        box3f(embree::empty) */
      virtual box3f getBounds() { 
        box3f bounds = embree::empty;
        for (int i=0;i<node.size();i++)
          bounds.extend(node[i]->getBounds());
        return bounds;
      }

    };
      
    
  } // ::ospray::sg
} // ::ospray


