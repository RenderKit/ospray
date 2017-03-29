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

#undef NDEBUG

#include "common/xml/XML.h"
#include "Model.h"
// embree
#include "ospcommon/FileName.h"
// std
#include <sstream>

namespace ospray {
  namespace particle {

    bool big_endian = false;

    /*! a dump-file we can use for debugging; we'll simply dump each
      parsed particle into this file during parsing */
    FILE *particleDumpFile = NULL;
    size_t numDumpedParticles = 0;

    struct Particle {
      double x,y,z;
    };

    double htonlf(double f)
    {
      double ret;
      char *in = (char*)&f;
      char *out = (char*)&ret;
      for (int i=0;i<8;i++)
        out[i] = in[7-i];
      return ret;
    }
    float htonf(float f)
    {
      float ret;
      char *in = (char*)&f;
      char *out = (char*)&ret;
      for (int i=0;i<4;i++)
        out[i] = in[3-i];
      return ret;
    }
    void readParticles(Model *model,
                       size_t numParticles, const std::string &fn, size_t begin, size_t end)
    {
      // std::cout << "#mpm: reading " << numParticles << " particles... " << std::flush;
      // printf("#mpm: reading%7ld particles...",numParticles); fflush(0);
      FILE *file = fopen(fn.c_str(),"rb");
      if (!file) {
        throw std::runtime_error("could not open data file "+fn);
      }
      assert(file);

      fseek(file,begin,SEEK_SET);
      size_t len = end-begin;

      if (len != numParticles*sizeof(Particle)) {
        PING;
        PRINT(len);
        PRINT(numParticles);
        PRINT(len/numParticles);
      }
      // PRINT(len);
      
      for (uint32_t i = 0; i < numParticles; i++) {
        Particle p;
        int rc = fread(&p,sizeof(p),1,file);
        if (rc != 1) {
          fclose(file);
          throw std::runtime_error("read partial data "+fn);
        }
#if 1
        if (big_endian) {
          p.x = htonlf(p.x);
          p.y = htonlf(p.y);
          p.z = htonlf(p.z);
        }
#endif
        Model::Atom a;
        a.position = vec3f(p.x,p.y,p.z);
        a.type = model->getAtomType("<unnamed>");

        if (particleDumpFile) {
          numDumpedParticles++;
          fwrite(&a,sizeof(a),1,particleDumpFile);
        } else 
          model->atom.push_back(a);
      }
      
      std::cout << "\r#osp:uintah: read " << numParticles << " particles (total " << float((numDumpedParticles+model->atom.size())/1e6) << "M)";

      fclose(file);
    }




    void readDoubleAttributes(Model *model, const std::string &attrName, size_t numParticles, 
                              const std::string &fn, size_t begin, size_t end)
    {
      // std::cout << "#mpm: reading " << numParticles << " particles... " << std::flush;
      // printf("#mpm: reading%7ld particles...",numParticles); fflush(0);
      FILE *file = fopen(fn.c_str(),"rb");
      if (!file) {
        throw std::runtime_error("could not open data file "+fn);
      }
      assert(file);

      fseek(file,begin,SEEK_SET);
      size_t len = end-begin;

      if (len != numParticles*sizeof(double)) {
        PING;
        PRINT(len);
        PRINT(numParticles);
        PRINT(len/numParticles);
      }
      // PRINT(len);
      
      for (uint32_t i = 0; i < numParticles; i++) {
        double attrib;
        int rc = fread(&attrib,sizeof(attrib),1,file);
        if (rc != 1) {
          fclose(file);
          throw std::runtime_error("read partial data "+fn);
        }
        if (big_endian) {
          attrib = htonlf(attrib);
        }
        model->addAttribute(attrName,attrib);
      }
      
      std::stringstream attrs;
      for (auto it = model->attribute.begin();
           it != model->attribute.end();
           it++) {
        attrs << "," << it->first;
      }
        

      std::cout << "\r#osp:uintah: " << numParticles << " pt.s (tot:" << float((numDumpedParticles+model->atom.size())/1e6) << "M" << attrs.str() << ")";

      fclose(file);
    }


    void readFloatAttributes(Model *model, const std::string &attrName, size_t numParticles, 
                             const std::string &fn, size_t begin, size_t end)
    {
      FILE *file = fopen(fn.c_str(),"rb");
      if (!file) {
        throw std::runtime_error("could not open data file "+fn);
      }
      assert(file);

      fseek(file,begin,SEEK_SET);
      size_t len = end-begin;

      if (len != numParticles*sizeof(float)) {
        PING;
        PRINT(len);
        PRINT(numParticles);
        PRINT(len/numParticles);
      }
      // PRINT(len);
      
      for (uint32_t i = 0; i < numParticles; i++) {
        float attrib;
        int rc = fread(&attrib,sizeof(attrib),1,file);
        if (rc != 1) {
          fclose(file);
          throw std::runtime_error("read partial data "+fn);
        }
#if 1
        if (big_endian) {
          attrib = htonf(attrib);
        }
#endif

        model->addAttribute(attrName,attrib);
      }
      
      std::stringstream attrs;
      for (std::map<std::string,std::vector<float> *>::const_iterator it=model->attribute.begin();
           it != model->attribute.end();it++) {
        attrs << "," << it->first;
      }
        

      std::cout << "\r#osp:uintah: " << numParticles << " pt.s (tot:" << float((numDumpedParticles+model->atom.size())/1e6) << "M" << attrs.str() << ")";

      fclose(file);
    }


    void parse__Variable(Model *model,
                         const std::string &basePath,
                         const xml::Node &var)
    {
      size_t index = -1;
      size_t start = -1;
      size_t end = -1;
      size_t patch = -1;
      size_t numParticles = 0;
      std::string variable;
      std::string filename;
      std::string varType = var.getProp("type");
      xml::for_each_child_of(var,[&](const xml::Node &child){
          if (child.name == "index") {
            index = atoi(child.content.c_str());
          } else if (child.name == "variable") {
            variable = child.content;
          } else if (child.name == "numParticles") {
            numParticles = atol(child.content.c_str());
          } else if (child.name == "patch") {
            patch = atol(child.content.c_str());
          } else if (child.name == "filename") {
            filename = child.content;
          } else if (child.name == "start") {
            start = atol(child.content.c_str());
          } else if (child.name == "end") {
            end = atol(child.content.c_str());
          }
        });
      
      (void)index;
      (void)patch;

      if (numParticles > 0
          && variable == "p.x"
          /* && index == .... */ 
          ) {
        readParticles(model,numParticles,basePath+"/"+filename,start,end);
      }
      else if (numParticles > 0
               && varType == "ParticleVariable&lt;double&gt;"
               ) { 
        readDoubleAttributes(model,variable,numParticles,basePath+"/"+filename,start,end);
      }
      else if (numParticles > 0
               && varType == "ParticleVariable&lt;float&gt;"
               ) { 
        readFloatAttributes(model,variable,numParticles,basePath+"/"+filename,start,end);
      }
    }

    void parse__Uintah_Datafile(Model *model,
                                const std::string &fileName)
    {
      std::string basePath = FileName(fileName).path();

      std::shared_ptr<xml::XMLDoc> doc;
      try {
        doc = xml::readXML(fileName);
      } catch (std::runtime_error e) {
        model->cullPartialData();
        static bool warned = false;
        if (!warned) {
          std::cerr << "#osp:uintah: error in opening xml data file: " << e.what() << std::endl;
          std::cerr << "#osp:uintah: continuing parsing, but parts of the data will be missing" << std::endl;
          std::cerr << "#osp:uintah: (only printing first instance of this error; there may be more)" << std::endl;
          warned = true;
        }
        return;
      }
      assert(doc);
      assert(doc->child.size() == 1);
      const xml::Node &node = *doc->child[0];
      assert(node.name == "Uintah_Output");
      xml::for_each_child_of(node,[&](const xml::Node &child){
          assert(child.name == "Variable");
          parse__Variable(model,basePath,child);
        });
    }
    
    void parse__Uintah_TimeStep_Data(Model *model,
                                     const std::string &basePath, const xml::Node &node)
    {
      assert(node.name == "Data");
      xml::for_each_child_of(node,[&](const xml::Node &child){
          assert(child.name == "Datafile");
          try {
            parse__Uintah_Datafile(model,basePath+"/"+node.getProp("href"));
          } catch (std::runtime_error e) {
            static bool warned = false;
            if (!warned) {
              std::cerr << "#osp:uintah: error in parsing timestep data: " << e.what() << std::endl;
              std::cerr << "#osp:uintah: continuing parsing, but parts of the data will be missing" << std::endl;
              std::cerr << "#osp:uintah: (only printing first instance of this error; there may be more)" << std::endl;
              warned = true;
            }
          }
        });
    }
    
    void parse__Uintah_TimeStep_Meta(Model */*model*/,
                                     const std::string &/*basePath*/,
                                     const xml::Node &node)
    {
      assert(node.name == "Meta");
      xml::for_each_child_of(node,[&](const xml::Node &child){
          if (child.name == "endianness") {
            if (child.content == "big_endian") {
              std::cout << "#osp:uintah: SWITCHING TO BIG_ENDIANNESS" << std::endl;
              big_endian = true;
            }
          }
        });
    }
    
    void parse__Uintah_timestep(Model *model,
                                const std::string &basePath, const xml::Node &node)
    {
      assert(node.name == "Uintah_timestep");
      xml::for_each_child_of(node,[&](const xml::Node &child){
          if (child.name == "Meta") {
            parse__Uintah_TimeStep_Meta(model,basePath,child);
          }
          if (child.name == "Data") {
            parse__Uintah_TimeStep_Data(model,basePath,child);
          }
        });
    }
    
    Model *parse__Uintah_timestep_xml(const std::string &s)
    {
      Model *model = new Model;
      Model::defaultRadius = .002f;
      std::shared_ptr<xml::XMLDoc> doc = xml::readXML(s);

      char *dumpFileName = getenv("OSPRAY_PARTICLE_DUMP_FILE");
      if (dumpFileName)
        particleDumpFile = fopen(dumpFileName,"wb");
      assert(doc);
      assert(doc->child.size() == 1);
      assert(doc->child[0]->name == "Uintah_timestep");
      std::string basePath = FileName(s).path();
      parse__Uintah_timestep(model, basePath, *doc->child[0]);

      std::stringstream attrs;
      for (std::map<std::string,std::vector<float> *>::const_iterator it=model->attribute.begin();
           it != model->attribute.end();it++) {
        attrs << ":" << it->first;
      }
        
      std::cout << "#osp:mpm: read " << s << " : " 
                << model->atom.size() << " particles (" << attrs.str() << ")" << std::endl;

      box3f bounds = empty;
      for (const auto & atom : model->atom) {
        bounds.extend(atom.position);
      }
      std::cout << "#osp:mpm: bounds of particle centers: " << bounds << std::endl;
      return model;
    }

  } // ::ospray::particle
} // ::ospray

