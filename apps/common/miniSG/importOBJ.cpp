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

#include "miniSG.h"
#include "importer.h"
#include <fstream>
#include <cmath>
#include <string>

/*! the boeing 777 model does not actually have a 'mtl' file; instead,
  as materials it has a single RBG diffuse color that's encoded in
  the material name used in 'usematerial' (eg, "usematerial
  Material255_0_0" is red). If this flag is turned on, we'll detect
  this case and create 'proper' obj materials in this parser, so any
  following tools don't have to care about this.*/
#define BOING_HACK 1


namespace ospray {
  namespace miniSG {
    using std::cout;
    using std::endl;

    /*! Three-index vertex, indexing start at 0, -1 means invalid vertex. */
    struct Vertex {
      int v, vt, vn;
      Vertex() {}
      Vertex(int v) : v(v), vt(v), vn(v) {}
      Vertex(int v, int vt, int vn) : v(v), vt(vt), vn(vn) {}
    };
    
    static inline bool operator < ( const Vertex& a, const Vertex& b ) {
      if (a.v  != b.v)  return a.v  < b.v;
      if (a.vn != b.vn) return a.vn < b.vn;
      if (a.vt != b.vt) return a.vt < b.vt;
      return false;
    }
    
    /*! Fill space at the end of the token with 0s. */
    static inline const char* trimEnd(const char* token) {
      size_t len = strlen(token);
      if (len == 0) return token;
      char* pe = (char*)(token + len - 1);
      while ((*pe == ' ' || *pe == '\t' || *pe == '\r') && pe >= token) {
        *pe-- = 0;
      }
      return token;
    }
    
    /*! Determine if character is a separator. */
    static inline bool isSep(const char c) {
      return (c == ' ') || (c == '\t');
    }
    
    /*! Parse separator. */
    static inline const char* parseSep(const char*& token) {
      size_t sep = strspn(token, " \t");
      if (!sep) throw std::runtime_error("separator expected");
      return token+=sep;
    }
    
    /*! Parse optional separator. */
    static inline const char* parseSepOpt(const char*& token) {
      return token+=strspn(token, " \t");
    }
    
    /*! Read float from a string. */
    static inline float getFloat(const char*& token) {
      token += strspn(token, " \t");
      float n = (float)atof(token);
      token += strcspn(token, " \t\r");
      return n;
    }
    
    /*! Read vec2f from a string. */
    static inline vec2f getVec2f(const char*& token) {
      float x = getFloat(token);
      float y = getFloat(token);
      return vec2f(x,y);
    }
    
    /*! Read vec3f from a string. */
    static inline vec3f getVec3f(const char*& token) {
      float x = getFloat(token);
      float y = getFloat(token);
      float z = getFloat(token);
      return vec3f(x,y,z);
    }
    
    class OBJLoader
    {
    public:
      
      Model &model;//ImportHelper importer;
      // std::vector<Handle<Device::RTPrimitive> > model;
      std::map<std::string,Material *> material;
      
      /*! Constructor. */
      OBJLoader(Model &model, const ospcommon::FileName& fileName);
      
      /*! Destruction */
      ~OBJLoader();
 
      /*! Public methods. */
      void loadMTL(const ospcommon::FileName& fileName);

    private:

      ospcommon::FileName path;

      /*! Geometry buffer. */
      std::vector<vec3f> v;
      std::vector<vec3f> vn;
      std::vector<vec2f> vt;
      std::vector<std::vector<Vertex> > curGroup;

      /*! Material handling. */
      Material *curMaterial;
      Material *defaultMaterial;

      /*! Internal methods. */
      int fix_v (int index);
      int fix_vt(int index);
      int fix_vn(int index);
      void flushFaceGroup();
      Vertex getInt3(const char*& token);
      uint32_t getVertex(std::map<Vertex,uint32_t>& vertexMap,
                         Mesh *mesh,
                         const Vertex& i);
    };

    OBJLoader::OBJLoader(Model &model, const ospcommon::FileName &fileName) :
      model(model),
      path(fileName.path()),
      curMaterial(nullptr)
    {
      /* open file */
      std::ifstream cin;
      cin.open(fileName.c_str());
      if (!cin.is_open()) {
        std::cerr << "cannot open " << fileName.str() << std::endl;
        return;
      }

      // /* generate default material */
      defaultMaterial = nullptr;
      curMaterial = defaultMaterial;

      char line[1000000];
      memset(line, 0, sizeof(line));

      while (cin.peek() != -1)
        {
          /* load next multiline */
          char* pline = line;
          while (true) {
            cin.getline(pline, sizeof(line) - (pline - line) - 16, '\n');
            ssize_t last = strlen(pline) - 1;
            if (last < 0 || pline[last] != '\\') break;
            pline += last;
            *pline++ = ' ';
          }

          const char* token = trimEnd(line + strspn(line, " \t"));
          if (token[0] == 0) continue;

          /*! parse position */
          if (token[0] == 'v' && isSep(token[1]))
          { v.push_back(getVec3f(token += 2)); continue; }

          /* parse normal */
          if (token[0] == 'v' && token[1] == 'n' && isSep(token[2]))
          { vn.push_back(getVec3f(token += 3)); continue; }

          /* parse texcoord */
          if (token[0] == 'v' && token[1] == 't' && isSep(token[2]))
          { vt.push_back(getVec2f(token += 3)); continue; }

          /*! parse face */
          if (token[0] == 'f' && isSep(token[1]))
            {
              parseSep(token += 1);

              std::vector<Vertex> face;
              while (token[0]) {
                face.push_back(getInt3(token));
                parseSepOpt(token);
              }
              curGroup.push_back(face);
              continue;
            }

          /*! use material */
          if (!strncmp(token, "usemtl", 6) && isSep(token[6]))
            {
              flushFaceGroup();
              std::string name(parseSep(token += 6));
              if (material.find(name) == material.end())
                curMaterial = defaultMaterial;
              else
                curMaterial = material[name];
              continue;
            }

          /* load material library */
          if (!strncmp(token, "mtllib", 6) && isSep(token[6])) {
            loadMTL(path + std::string(parseSep(token += 6)));
            continue;
          }
      
          // ignore unknown stuff
        }
      flushFaceGroup();
      cin.close();
    }
    
    OBJLoader::~OBJLoader()
    {
    }

    /* load material file */
    void OBJLoader::loadMTL(const ospcommon::FileName &fileName)
    {
      std::ifstream cin;
      cin.open(fileName.c_str());
      if (!cin.is_open()) {
        std::cerr << "cannot open " << fileName.str() << std::endl;
        return;
      }

      char line[10000];
      memset(line, 0, sizeof(line));

      Material *cur = nullptr;
      while (cin.peek() != -1)
        {
          /* load next multiline */
          char* pline = line;
          while (true) {
            cin.getline(pline, sizeof(line) - (pline - line) - 16, '\n');
            ssize_t last = strlen(pline) - 1;
            if (last < 0 || pline[last] != '\\') break;
            pline += last;
            *pline++ = ' ';
          }
          const char* token = trimEnd(line + strspn(line, " \t"));

          if (token[0] == 0  ) continue; // ignore empty lines
          if (token[0] == '#') continue; // ignore comments

          if (!strncmp(token, "newmtl", 6)) {
            parseSep(token+=6);
            // if (cur) g_device->rtCommit(cur);
            std::string name(token);
            material[name] = cur = new Material; //g_device->rtNewMaterial("obj");
            //            model.material.push_back(cur);
            cur->name = name;
            cur->type = "OBJ";
            continue;
          }

          if (!cur) {
            //throw std::runtime_error("invalid material file: newmtl expected first");
            cout << "#osp:minisg:parseOBJ (Warning): cannot parse input line " << line << endl;
            continue;
          }

          if (!strncmp(token, "illum_4",7)) { 
            /*! iw: hack for VMD-exported OBJ files, working ardouna
                bug in VMD's OBJ exporter (VMD writes "illum_4" (with
                an underscore) rather than "illum 4" (with a
                whitespace) */
            parseSep(token += 7);  
            continue; 
          }

          if (!strncmp(token, "illum", 5)) { parseSep(token += 5);  continue; }

          if (!strncmp(token, "d",  1)) { parseSep(token += 1);  cur->setParam("d", getFloat(token));  continue; }
          if (!strncmp(token, "Ns", 2)) { parseSep(token += 2);  cur->setParam("Ns", getFloat(token));  continue; }
          if (!strncmp(token, "Ni", 2)) { parseSep(token += 2);  cur->setParam("Ni", getFloat(token));  continue; }

          if (!strncmp(token, "Ka", 2)) { parseSep(token += 2);  cur->setParam("Ka", getVec3f(token)); continue; }
          if (!strncmp(token, "Kd", 2)) { parseSep(token += 2);  cur->setParam("Kd", getVec3f(token)); continue; }
          if (!strncmp(token, "Ks", 2)) { parseSep(token += 2);  cur->setParam("Ks", getVec3f(token)); continue; }
          if (!strncmp(token, "Tf", 2)) { parseSep(token += 2);  cur->setParam("Tf", getVec3f(token)); continue; }

          if (!strncmp(token, "map_d" , 5)) { parseSepOpt(token += 5);  cur->setParam("map_d", loadTexture(path, std::string(token), true),Material::Param::TEXTURE);  continue; }
          if (!strncmp(token, "map_Ns" , 6)) { parseSepOpt(token += 6); cur->setParam("map_Ns", loadTexture(path, std::string(token), true),Material::Param::TEXTURE);  continue; }
          if (!strncmp(token, "map_Ka" , 6)) { parseSepOpt(token += 6); cur->setParam("map_Ka", loadTexture(path, std::string(token)),Material::Param::TEXTURE);  continue; }
          if (!strncmp(token, "map_Kd" , 6)) { parseSepOpt(token += 6); cur->setParam("map_Kd", loadTexture(path, std::string(token)),Material::Param::TEXTURE);  continue; }
          if (!strncmp(token, "map_Ks" , 6)) { parseSepOpt(token += 6); cur->setParam("map_Ks", loadTexture(path, std::string(token)),Material::Param::TEXTURE);  continue; }
          /*! the following are extensions to the standard */
          if (!strncmp(token, "map_Refl" , 8)) { parseSepOpt(token += 8);  cur->setParam("map_Refl", loadTexture(path, std::string(token)),Material::Param::TEXTURE);  continue; }
          if (!strncmp(token, "map_Bump" , 8)) { parseSepOpt(token += 8);  cur->setParam("map_Bump", loadTexture(path, std::string(token), true),Material::Param::TEXTURE);  continue; }

          if (!strncmp(token, "bumpMap" , 7)) { parseSepOpt(token += 7);  cur->setParam("map_Bump", loadTexture(path, std::string(token), true),Material::Param::TEXTURE);  continue; }
          if (!strncmp(token, "colorMap" , 8)) { parseSepOpt(token += 8);  cur->setParam("map_Kd", loadTexture(path, std::string(token)),Material::Param::TEXTURE);  continue; }

          if (!strncmp(token, "color", 5)) { parseSep(token += 5);  cur->setParam("color", getVec3f(token)); continue; }
          if (!strncmp(token, "type", 4)) { parseSep(token += 4);  cur->type = std::string(token); cur->setParam("type", token); continue; }

          // add anything else as float param
          const char * ident = token;
          token += strcspn(token, " \t");
          *(char*)token = 0;
          parseSepOpt(token += 1);
          cur->setParam(ident, getFloat(token));
        }
      // if (cur) g_device->rtCommit(cur);
      cin.close();
    }

    /*! handles relative indices and starts indexing from 0 */
    int OBJLoader::fix_v (int index) { return(index > 0 ? index - 1 : (index == 0 ? 0 : (int) v .size() + index)); }
    int OBJLoader::fix_vt(int index) { return(index > 0 ? index - 1 : (index == 0 ? 0 : (int) vt.size() + index)); }
    int OBJLoader::fix_vn(int index) { return(index > 0 ? index - 1 : (index == 0 ? 0 : (int) vn.size() + index)); }

    /*! Parse differently formated triplets like: n0, n0/n1/n2, n0//n2, n0/n1.          */
    /*! All indices are converted to C-style (from 0). Missing entries are assigned -1. */
    Vertex OBJLoader::getInt3(const char*& token)
    {
      Vertex v(-1);
      v.v = fix_v(atoi(token));
      token += strcspn(token, "/ \t\r");
      if (token[0] != '/') return(v);
      token++;

      // it is i//n
      if (token[0] == '/') {
        token++;
        v.vn = fix_vn(atoi(token));
        token += strcspn(token, " \t\r");
        return(v);
      }

      // it is i/t/n or i/t
      v.vt = fix_vt(atoi(token));
      token += strcspn(token, "/ \t\r");
      if (token[0] != '/') return(v);
      token++;

      // it is i/t/n
      v.vn = fix_vn(atoi(token));
      token += strcspn(token, " \t\r");
      return(v);
    }

    uint32_t OBJLoader::getVertex(std::map<Vertex,uint32_t>& vertexMap, 
                                Mesh *mesh, const Vertex& i)
    {
      const std::map<Vertex, uint32_t>::iterator& entry = vertexMap.find(i);
      if (entry != vertexMap.end()) return(entry->second);

      if (std::isnan(v[i.v].x) || std::isnan(v[i.v].y) || std::isnan(v[i.v].z))
        return -1;

      if (i.vn >= 0 && (std::isnan(vn[i.vn].x) ||
                        std::isnan(vn[i.vn].y) ||
                        std::isnan(vn[i.vn].z)))
        return -1;

      if (i.vt >= 0 && (std::isnan(vt[i.vt].x) ||
                        std::isnan(vt[i.vt].y)))
        return -1;

      mesh->position.push_back(v[i.v]);
      if (i.vn >= 0) mesh->normal.push_back(vn[i.vn]);
      if (i.vt >= 0) mesh->texcoord.push_back(vt[i.vt]);
      return(vertexMap[i] = int(mesh->position.size()) - 1);
    }

    /*! end current facegroup and append to mesh */
    void OBJLoader::flushFaceGroup()
    {
      if (curGroup.empty()) return;

      std::map<Vertex, uint32_t> vertexMap;
      Mesh *mesh = new Mesh;
      model.mesh.push_back(mesh);
      model.instance.push_back(Instance(model.mesh.size()-1));
      mesh->material = curMaterial;
      // merge three indices into one
      for (size_t j=0; j < curGroup.size(); j++)
        {
          /* iterate over all faces */
          const std::vector<Vertex>& face = curGroup[j];
          Vertex i0 = face[0], i1 = Vertex(-1), i2 = face[1];
          
          /* triangulate the face with a triangle fan */
          for (size_t k=2; k < face.size(); k++) {
            i1 = i2; i2 = face[k];
            int32_t v0 = getVertex(vertexMap, mesh, i0);
            int32_t v1 = getVertex(vertexMap, mesh, i1);
            int32_t v2 = getVertex(vertexMap, mesh, i2);
            if (v0 < 0 || v1 < 0 || v2 < 0)
              continue;
            Triangle tri;
            tri.v0 = v0;
            tri.v1 = v1;
            tri.v2 = v2;
            mesh->triangle.push_back(tri);
          }
        }
      curGroup.clear();
    }

    void importOBJ(Model &model,
                   const ospcommon::FileName &fileName)
    {
      OBJLoader(model,fileName);
    }

  } // ::ospray::minisg
} // ::ospray


