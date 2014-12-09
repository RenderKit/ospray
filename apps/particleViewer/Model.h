#pragma once

// ospray
#include "common/OSPCommon.h"
// stl
#include <map>
#include <vector>

namespace ospray {
  namespace particle {

    struct Model {
      struct AtomType {
        const std::string name;
        vec3f             color;

        AtomType(const std::string &name) : name(name), color(.7f) {};
      };
      struct Atom {
        vec3f position;
        int type; /*! type of the model in atomType - also serves as a material ID */
      };

      std::vector<AtomType *>   atomType;
      std::map<std::string,int> atomTypeByName;
      std::vector<Atom> atom;
      float radius;

      //! list of attribute values, if applicable.
      std::map<std::string,std::vector<float> *> attribute;

      int getAtomType(const std::string &name);

      /*! \brief load lammps xyz files */
      void loadXYZ(const std::string &fn);
      /*! load xyz files in which there is *no* atom count, but just a
        list of "type x y z" lines */
      void loadXYZ2(const std::string &fileName);

      box3f getBBox() const;

      Model() : radius(1.f) {}

      void addAttribute(const std::string &name, float value)
      {
        if (!attribute[name]) attribute[name] = new std::vector<float>;
        attribute[name]->push_back(value);
      }

      void savePositions(const std::string &fileName)
      {
        FILE *file = fopen(fileName.c_str(),"wb");
      }
      void saveAttribute(const std::string &fileName, const std::vector<float> &attr)
      {
        FILE *file = fopen(fileName.c_str(),"wb");
        for (int i=0;i<attr.size();i++)
          fwrite(&attr[i],sizeof(float),1,file);
        fclose(file);
      }
      void saveToFile(const std::string &fileName) {
        const std::string binName = fileName+".bin";
        
        FILE *txt = fopen(fileName.c_str(),"w");
        FILE *bin = fopen(binName.c_str(),"wb");
        
        fprintf(txt,"atoms %li offset %li\n",atom.size(),ftell(bin));
        for (int i=0;i<atom.size();i++)
          fwrite(&atom[i].position,sizeof(vec3f),1,bin);

        for (std::map<std::string,std::vector<float> *>::const_iterator it=attribute.begin();
             it != attribute.end();it++) {
          fprintf(txt,"attribute offset %li name %s\n",
                  ftell(bin),it->first.c_str());
          fwrite(&*it->second->begin(),sizeof(float),it->second->size(),bin);
        }

        fclose(txt);
        fclose(bin);
      }

      void cullPartialData() 
      {
        size_t largestCompleteSize = atom.size();
        for (std::map<std::string,std::vector<float> *>::const_iterator it=attribute.begin();
             it != attribute.end();it++)
          largestCompleteSize = std::min(largestCompleteSize,it->second->size());
        
        if (atom.size() > largestCompleteSize) {
          std::cout << "#osp:uintah: atoms w missing attribute(s): discarding" << std::endl;
          atom.resize(largestCompleteSize);
        }
        for (std::map<std::string,std::vector<float> *>::const_iterator it=attribute.begin();
             it != attribute.end();it++) {
          if (it->second->size() > largestCompleteSize) {
            std::cout << "#osp:uintah: attribute(s) w/o atom(s): discarding" << std::endl;
            it->second->resize(largestCompleteSize);
          }
        }
      }
    };

  } // ::ospray::particle
} // ::ospray
