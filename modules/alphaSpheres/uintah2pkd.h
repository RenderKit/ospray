/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */
#pragma once

// ospray
#include "common/OspCommon.h"
// stl
#include <map>
#include <vector>

namespace ospray {
  namespace particle {

    struct Model {
      struct Particle {
        vec3f position;
        float attribute;
      };

      std::vector<Particle> particle;
      float radius;

      //! list of attribute values, if applicable.
      std::map<std::string,std::vector<float> *> attribute;

      box3f getBBox() const;

      Model() : radius(1.f) {}

      void addAttribute(const std::string &name, float value)
      {
        if (!attribute[name]) attribute[name] = new std::vector<float>;
        attribute[name]->push_back(value);
      }

      // void savePositions(const std::string &fileName)
      // {
      //   FILE *file = fopen(fileName.c_str(),"wb");
      // }
      // void saveAttribute(const std::string &fileName, const std::vector<float> &attr)
      // {
      //   FILE *file = fopen(fileName.c_str(),"wb");
      //   for (int i=0;i<attr.size();i++)
      //     fwrite(&attr[i],sizeof(float),1,file);
      //   fclose(file);
      // }
      // void saveToFile(const std::string &fileName) {
      //   const std::string binName = fileName+".bin";
        
      //   FILE *txt = fopen(fileName.c_str(),"w");
      //   FILE *bin = fopen(binName.c_str(),"wb");
        
      //   fprintf(txt,"particles %li offset %li\n",particle.size(),ftell(bin));
      //   for (int i=0;i<particle.size();i++)
      //     fwrite(&particle[i].position,sizeof(vec3f),1,bin);

      //   for (std::map<std::string,std::vector<float> *>::const_iterator it=attribute.begin();
      //        it != attribute.end();it++) {
      //     fprintf(txt,"attribute offset %li name %s\n",
      //             ftell(bin),it->first.c_str());
      //     fwrite(&*it->second->begin(),sizeof(float),it->second->size(),bin);
      //   }

      //   fclose(txt);
      //   fclose(bin);
      // }

      void cullPartialData() 
      {
        size_t largestCompleteSize = particle.size();
        for (std::map<std::string,std::vector<float> *>::const_iterator it=attribute.begin();
             it != attribute.end();it++)
          largestCompleteSize = std::min(largestCompleteSize,it->second->size());
        
        if (particle.size() > largestCompleteSize) {
          std::cout << "#osp:uintah: particles w missing attribute(s): discarding" << std::endl;
          particle.resize(largestCompleteSize);
        }
        for (std::map<std::string,std::vector<float> *>::const_iterator it=attribute.begin();
             it != attribute.end();it++) {
          if (it->second->size() > largestCompleteSize) {
            std::cout << "#osp:uintah: attribute(s) w/o particle(s): discarding" << std::endl;
            it->second->resize(largestCompleteSize);
          }
        }
      }
    };

  }
}
