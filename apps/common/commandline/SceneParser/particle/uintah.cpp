// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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
      
      for (int i=0;i<numParticles;i++) {
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
      
      for (int i=0;i<numParticles;i++) {
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
      for (std::map<std::string,std::vector<float> *>::const_iterator it=model->attribute.begin();
           it != model->attribute.end();it++) {
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
      
      for (int i=0;i<numParticles;i++) {
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
                         const std::string &basePath, xml::Node *var)
    {
      size_t index = -1;
      size_t start = -1;
      size_t end = -1;
      size_t patch = -1;
      size_t numParticles = 0;
      std::string variable;
      std::string filename;
      std::string varType = var->getProp("type");
      for (int i=0;i<var->child.size();i++) {
        xml::Node *n = var->child[i];
        if (n->name == "index") {
          index = atoi(n->content.c_str());
        } else if (n->name == "variable") {
          variable = n->content;
        } else if (n->name == "numParticles") {
          numParticles = atol(n->content.c_str());
        } else if (n->name == "patch") {
          patch = atol(n->content.c_str());
        } else if (n->name == "filename") {
          filename = n->content;
        } else if (n->name == "start") {
          start = atol(n->content.c_str());
        } else if (n->name == "end") {
          end = atol(n->content.c_str());
        }
      }

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

      xml::XMLDoc *doc = NULL;
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
      xml::Node *node = doc->child[0];
      assert(node->name == "Uintah_Output");
      for (int i=0;i<node->child.size();i++) {
        xml::Node *c = node->child[i];
        assert(c->name == "Variable");
        parse__Variable(model,basePath,c);
      }
      delete doc;
    }
    void parse__Uintah_TimeStep_Data(Model *model,
                                     const std::string &basePath, xml::Node *node)
    {
      assert(node->name == "Data");
      for (int i=0;i<node->child.size();i++) {
        xml::Node *c = node->child[i];
        assert(c->name == "Datafile");
        for (int j=0;j<c->prop.size();j++) {
          xml::Prop *p = c->prop[j];
          if (p->name == "href") {
            try {
              parse__Uintah_Datafile(model,basePath+"/"+p->value);
            } catch (std::runtime_error e) {
              static bool warned = false;
              if (!warned) {
                std::cerr << "#osp:uintah: error in parsing timestep data: " << e.what() << std::endl;
                std::cerr << "#osp:uintah: continuing parsing, but parts of the data will be missing" << std::endl;
                std::cerr << "#osp:uintah: (only printing first instance of this error; there may be more)" << std::endl;
                warned = true;
              }
            }
          }
        }
      }
    }
    void parse__Uintah_TimeStep_Meta(Model *model,
                                     const std::string &basePath, xml::Node *node)
    {
      assert(node->name == "Meta");
      for (int i=0;i<node->child.size();i++) {
        xml::Node *c = node->child[i];
        if (c->name == "endianness") {
          if (c->content == "big_endian") {
            std::cout << "#osp:uintah: SWITCHING TO BIG_ENDIANNESS" << std::endl;
            big_endian = true;
          }
        }
      }
    }
    void parse__Uintah_timestep(Model *model,
                                const std::string &basePath, xml::Node *node)
    {
      assert(node->name == "Uintah_timestep");
      for (int i=0;i<node->child.size();i++) {
        xml::Node *c = node->child[i];
        if (c->name == "Meta") {
          parse__Uintah_TimeStep_Meta(model,basePath,c);
        }
        if (c->name == "Data") {
          parse__Uintah_TimeStep_Data(model,basePath,c);
        }
      }
    }
    Model *parse__Uintah_timestep_xml(const std::string &s)
    {
      Model *model = new Model;
      Model::defaultRadius = .002f;
      xml::XMLDoc *doc = xml::readXML(s);

      char *dumpFileName = getenv("OSPRAY_PARTICLE_DUMP_FILE");
      if (dumpFileName)
        particleDumpFile = fopen(dumpFileName,"wb");
      assert(doc);
      assert(doc->child.size() == 1);
      assert(doc->child[0]->name == "Uintah_timestep");
      std::string basePath = FileName(s).path();
      parse__Uintah_timestep(model, basePath, doc->child[0]);

      std::stringstream attrs;
      for (std::map<std::string,std::vector<float> *>::const_iterator it=model->attribute.begin();
           it != model->attribute.end();it++) {
        attrs << ":" << it->first;
      }
        
      std::cout << "#osp:mpm: read " << s << " : " 
                << model->atom.size() << " particles (" << attrs.str() << ")" << std::endl;

      box3f bounds = empty;
      for (int i=0;i<model->atom.size();i++) {
        bounds.extend(model->atom[i].position);
      }
      std::cout << "#osp:mpm: bounds of particle centers: " << bounds << std::endl;
      delete doc;
      return model;
    }

  } // ::ospray::particle
} // ::ospray

