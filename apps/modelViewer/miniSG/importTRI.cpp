/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "miniSG.h"
#include "importer.h"
#include <sys/times.h>

namespace ospray {
  namespace miniSG {
    using std::cout;
    using std::endl;

    void importTRI(Model &model,
                   const embree::FileName &fileName)
    {
      FILE *file = fopen(fileName.c_str(),"rb");
      if (!file) error("could not open input file");

      int32 numVertices;
      fread(&numVertices,1,sizeof(numVertices),file);

      Mesh *mesh = new Mesh;
      model.mesh.push_back(mesh);

      mesh->position.resize(numVertices);
      mesh->normal.resize(numVertices);
      mesh->triangle.resize(numVertices/3);
      fread(&mesh->position[0],numVertices,4*sizeof(float),file);
      fread(&mesh->normal[0],numVertices,4*sizeof(float),file);
      for (int i=0;i<numVertices/3;i++) {
        mesh->triangle[i].v0 = 3*i+0;
        mesh->triangle[i].v1 = 3*i+1;
        mesh->triangle[i].v2 = 3*i+2;
      }
      model.instance.push_back(Instance(0));
    }
  }
}


