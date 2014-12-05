/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

// run the parser with assertions, even in release mode:
#undef NDEBUG

#include "miniSG.h"
#include "importer.h"
#include "apps/common/xml/xml.h"
#include <fstream>

namespace ospray {
  namespace miniSG {
    using std::cout;
    using std::endl;
    
    void parseX3D(Model &model, xml::Node *root)
    {
      PRINT(root->child.size());
    }

    /*! import a list of X3D files */
    void importX3D(Model &model, 
                   const embree::FileName &fileName)
    {
      xml::XMLDoc *doc = xml::readXML(fileName);
      assert(doc);
      PRINT(doc->child[0]->name);
      if (doc->child.size() != 1 || doc->child[0]->name != "X3D") 
        throw std::runtime_error("could not parse X3D file: Not in X3D format!?");
      xml::Node *root_element = doc->child[0];
      parseX3D(model,root_element);
    }

  }
}
