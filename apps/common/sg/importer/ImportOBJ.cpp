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

#define WARN_INCLUDE_EMBREE_FILENAME 1

// O_LARGEFILE is a GNU extension.
#ifdef __APPLE__
#define  O_LARGEFILE  0
#endif

// sg
#include "SceneGraph.h"
#include "sg/common/Texture2D.h"
#include "sg/geometry/TriangleMesh.h"
#include <cmath>

#ifdef _WIN32
#include <string.h>
int strncasecmp(const char *s1, const char *s2, size_t n)
{ return _strnicmp(s1, s2, n); }
#endif

namespace ospray {
  namespace sg {
    using std::string;
    using std::cout;
    using std::endl;

    std::shared_ptr<Texture2D> loadTexture(const std::string &path,
                                           const std::string &fileName,
                                           const bool prefereLinear = false)
    {
      FileName texFileName = path+"/"+fileName;
      std::shared_ptr<Texture2D> tex = Texture2D::load(texFileName, prefereLinear);
      if (!tex)
        std::cout << "could not load texture " << texFileName << " !" << endl;
      return tex;
    }

    /*! Three-index vertex, indexing start at 0, -1 means invalid vertex. */
    struct Vertex {
      int v, vt, vn;
      Vertex() {};
      Vertex(int v) : v(v), vt(v), vn(v) {};
      Vertex(int v, int vt, int vn) : v(v), vt(vt), vn(vn) {};
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
      while ((*pe == ' ' || *pe == '\t' || *pe == '\r') && pe >= token)
        *pe-- = 0;
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
      
      std::shared_ptr<World> world;
      std::map<std::string,std::shared_ptr<Material> > material;
      
      /*! Constructor. */
      OBJLoader(std::shared_ptr<World> world, const FileName& fileName);
      
      /*! Destruction */
      ~OBJLoader();
 
      /*! Public methods. */
      void loadMTL(const FileName& fileName);
      
      template<typename T>
      inline T parse(const char *&token);

      /*! try to parse given token stream as a float-type with given
        keyword; and if successful, assign to material. returns true
        if matched, false if not */
      template<typename T>
      bool tryToMatch(const char *&token,
                      const char *keyWord,
                      const std::shared_ptr<Material> &mat);
      
      /*! try to parse given token stream as a float-type with given
        keyword; and if successful, load the texture and assign to
        material. returns true if matched, false if not */
      bool tryToMatchTexture(const char *&token,
                             const char *keyWord,
                             const std::shared_ptr<Material> &mat,
                             bool preferLinear=false);

    private:

      FileName path;
      FileName fullPath;

      /*! Geometry buffer. */
      std::vector<vec3f> v;
      std::vector<vec3f> vn;
      std::vector<vec2f> vt;
      std::vector<std::vector<Vertex> > curGroup;
      std::string curGroupName;

      /*! Material handling. */
      std::shared_ptr<Material> curMaterial;
      std::shared_ptr<Material> defaultMaterial;
      //std::map<std::string, Handle<Device::RTMaterial> > material;

      /*! Internal methods. */
      int fix_v (int index);
      int fix_vt(int index);
      int fix_vn(int index);
      void flushFaceGroup();
      Vertex getInt3(const char*& token);
      uint32_t getVertex(std::map<Vertex,uint32_t>& vertexMap,
                         const std::shared_ptr<TriangleMesh> &mesh,
                         const Vertex& i);
    };

    
    template<> inline vec3f OBJLoader::parse(const char *&token)
    { return getVec3f(token); }
    template<> inline float OBJLoader::parse(const char *&token)
    { return getFloat(token); }
    template<> inline std::string OBJLoader::parse(const char *&token)
    { return std::string(token); }

    OBJLoader::OBJLoader(std::shared_ptr<World> world, const FileName &fileName) 
      : world(world),
        path(fileName.path()),
        fullPath(fileName)
    {
      /* open file */
      std::ifstream cin;
      cin.open(fileName.c_str());
      if (!cin.is_open()) {
        std::cerr << "cannot open " << fileName.str() << std::endl;
        return;
      }

      // /* generate default material */
      // Handle<Device::RTMaterial> defaultMaterial = g_device->rtNewMaterial("matte");
      // g_device->rtSetFloat3(defaultMaterial, "reflectance", 0.5f, 0.5f, 0.5f);
      // g_device->rtCommit(defaultMaterial);
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

          if (token[0] == 'g')
            curGroupName = std::string(parseSep(token += 1));

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


    /*! try to parse given token stream as a float-type with given
      keyword; and if successful, assign to material. returns true
      if matched, false if not */
    template<typename T>
    bool OBJLoader::tryToMatch(const char *&token,
                               const char *keyWord,
                               const std::shared_ptr<Material> &mat)
    {
      if (strncasecmp(token, keyWord, strlen(keyWord)))
        return false;
      
      parseSep(token+=strlen(keyWord));
      mat->setParam(keyWord, parse<T>(token));

      return true;
    }
    
    /*! try to parse given token stream as a float-type with given
      keyword; and if successful, load the texture and assign to
      material. returns true if matched, false if not */
    bool OBJLoader::tryToMatchTexture(const char *&token,
                                      const char *keyWord,
                                      const std::shared_ptr<Material> &mat,
                                      bool preferLinear)
    {
      if (strncasecmp(token, keyWord, strlen(keyWord)))
        return false;

      parseSep(token+=strlen(keyWord));
      mat->setParam(keyWord,
                    loadTexture(path,parse<std::string>(token),preferLinear));
      return true;
    }

    /* load material file */
    void OBJLoader::loadMTL(const FileName &fileName)
    {
      std::ifstream cin;
      cin.open(fileName.c_str());
      if (!cin.is_open()) {
        std::cerr << "cannot open " << fileName.str() << std::endl;
        return;
      }

      char line[10000];
      memset(line, 0, sizeof(line));

      //      Handle<Device::RTMaterial> cur = null;
      std::shared_ptr<Material> cur;
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
            material[name] = cur = std::make_shared<Material>(); //g_device->rtNewMaterial("obj");
            //            model.material.push_back(cur);
            cur->name = name;
            cur->type = "OBJ";
            continue;
          }

          if (!cur) {
            std::cout << "#osp:sg:importOBJ: Ignoring line >> " << line << " <<"
                      << endl;
            continue;
          }//throw std::runtime_error("invalid material file: newmtl expected first");

          if (!strncmp(token, "illum_4",7)) { 
            /*! iw: hack for VMD-exported OBJ files, working around a
              bug in VMD's OBJ exporter (VMD writes "illum_4" (with
              an underscore) rather than "illum 4" (with a
              whitespace) */
            parseSep(token += 7);  
            continue; 
          }

          if (tryToMatch<float>(token,"illum",cur)) continue;
          if (tryToMatch<float>(token,"d",cur)) continue;
          if (tryToMatch<float>(token,"Ns",cur)) continue;
          if (tryToMatch<float>(token,"Ni",cur)) continue;
          if (tryToMatch<vec3f>(token,"Ka",cur)) continue;
          if (tryToMatch<vec3f>(token,"Kd",cur)) continue;
          if (tryToMatch<vec3f>(token,"Ks",cur)) continue;
          if (tryToMatch<vec3f>(token,"Tf",cur)) continue;

          if (tryToMatchTexture(token,"map_d",cur,true)) continue;
          if (tryToMatchTexture(token,"map_Ns",cur,true)) continue;
          if (tryToMatchTexture(token,"map_Ka",cur)) continue;
          if (tryToMatchTexture(token,"map_Kd",cur)) continue;
          if (tryToMatchTexture(token,"map_Ks",cur)) continue;

          /*! the following are extensions to the standard */
          if (tryToMatch<vec3f>(token,"color",cur)) continue;
          if (tryToMatchTexture(token,"map_Refl",cur)) continue;
          if (tryToMatchTexture(token,"map_Bump",cur)) continue;

          if (!strncmp(token, "type", 4)) {
            parseSep(token += 4);
            cur->type = std::string(token);
            continue;
          }

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
    int OBJLoader::fix_v (int index)
    { return(index > 0 ? index - 1 : (index == 0 ? 0 : (int) v .size() + index)); }
    int OBJLoader::fix_vt(int index)
    { return(index > 0 ? index - 1 : (index == 0 ? 0 : (int) vt.size() + index)); }
    int OBJLoader::fix_vn(int index)
    { return(index > 0 ? index - 1 : (index == 0 ? 0 : (int) vn.size() + index)); }

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
                                  const std::shared_ptr<TriangleMesh> &mesh,
                                  const Vertex& i)
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

      std::dynamic_pointer_cast<DataVector3f>(mesh->vertex)->push_back(v[i.v]);
      if (i.vn >= 0) //mesh->normal.cast<DataVector3f>()->v.push_back(vn[i.vn]);
        std::dynamic_pointer_cast<DataVector3f>(mesh->normal)->push_back(vn[i.vn]);
      if (i.vt >= 0) //mesh->texcoord.cast<DataVector2f>()->v.push_back(vt[i.vt]);
        std::dynamic_pointer_cast<DataVector2f>(mesh->texcoord)->push_back(vt[i.vt]);
      return(vertexMap[i] = int(mesh->vertex->getSize()) - 1);
    }

    /*! end current facegroup and append to mesh */
    void OBJLoader::flushFaceGroup()
    {
      if (curGroup.empty()) return;

      std::map<Vertex, uint32_t> vertexMap;
      std::string name = fullPath.name()+"_"+curGroupName;
      //scenegraph
      std::shared_ptr<TriangleMesh> mesh = std::static_pointer_cast<TriangleMesh>(createNode(name, "TriangleMesh").get());
      world->add(mesh);
      mesh->vertex = std::make_shared<DataVector3f>();
      mesh->normal = std::make_shared<DataVector3f>();
      mesh->texcoord = std::make_shared<DataVector2f>();
      mesh->index =  std::make_shared<DataVector3i>();
      mesh->material = curMaterial;

      // merge three indices into one
      for (size_t j=0; j < curGroup.size(); j++) {
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

          vec3i tri(v0,v1,v2);
          std::dynamic_pointer_cast<DataVector3i>(mesh->index)->push_back(tri); //Vec3i(v0, v1, v2));
        }
      }
      curGroup.clear();
    }

    void importOBJ(const std::shared_ptr<World> &world, const FileName &fileName)
    {
      std::cout << "ospray::sg::importOBJ: importing from " << fileName << endl;
      OBJLoader loader(world,fileName);
    }
  }
} // ::ospray

