/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "sg/common/Node.h"

namespace ospray {
  namespace sg {

    // list of all named nodes - for now use this as a global
    // variable, but eventually we'll need tofind a better way for
    // storing this
    std::map<std::string,Ref<sg::Node> > namedNodes;

    sg::Node *findNamedNode(const std::string &name)
    { 
      if (namedNodes.find(name) != namedNodes.end()) 
        return namedNodes[name].ptr; 
      return NULL; 
    }

    void registerNamedNode(const std::string &name, Ref<sg::Node> node)
    {
      namedNodes[name] = node; 
    }

  }
}
