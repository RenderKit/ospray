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

#include "StreamLineSceneParser.h"

#include "common/xml/XML.h"

using namespace ospray;
using namespace ospcommon;

#include <iostream>
using std::cout;
using std::endl;

// Helper types ///////////////////////////////////////////////////////////////

struct Triangles {
  std::vector<vec3fa> vertex;
  std::vector<vec4f>  color; // vertex color, from sv's 'v' value
  std::vector<vec3i>  index;

  struct SVVertex {
    float v;
    vec3f pos;
  };

  struct SVTriangle {
    SVVertex vertex[3];
  };

  vec3f lerpf(float x, float x0, float x1, vec3f y0, vec3f y1)
  {
    float f = (x-x0)/(x1-x0);
    return f*y1+(1-f)*y0;
  }
  
  vec4f colorOf(const float f)
  {
    if (f < .5f)
      return vec4f(lerpf(f, 0.f,.5f,vec3f(0),vec3f(0,1,0)), 1.f);
    else
      return vec4f(lerpf(f, .5f,1.f,vec3f(0,1,0),vec3f(1,0,0)), 1.f);
  }
  void parseSV(const FileName &fn)
  {
    FILE *file = fopen(fn.str().c_str(),"rb");
    if (!file) return;
    SVTriangle triangle;
    while (fread(&triangle,sizeof(triangle),1,file)) {
      index.push_back(vec3i(0,1,2)+vec3i(vertex.size()));
      vertex.push_back(vec3fa(triangle.vertex[0].pos));
      vertex.push_back(vec3fa(triangle.vertex[1].pos));
      vertex.push_back(vec3fa(triangle.vertex[2].pos));
      color.push_back(colorOf(triangle.vertex[0].v));
      color.push_back(colorOf(triangle.vertex[1].v));
      color.push_back(colorOf(triangle.vertex[2].v));
    }
    fclose(file);
  }

  box3f getBounds() const
  {
    box3f bounds = empty;
    for (const auto &v : vertex)
      bounds.extend(v);
    return bounds;
  }
};

struct StreamLines {
  std::vector<vec3fa> vertex;
  std::vector<int>    index;
  float radius {0.001f};

  void parsePNT(const FileName &fn)
  {
    FILE *file = fopen(fn.c_str(),"r");
    if (!file) {
      cout << "WARNING: could not open file " << fn << endl;
      return;
    }
    vec3fa pnt;
    static size_t totalSegments = 0;
    size_t segments = 0;
    // cout << "parsing file " << fn << ":" << std::flush;
    int rc = fscanf(file,"%f %f %f\n",&pnt.x,&pnt.y,&pnt.z);
    vertex.push_back(pnt);
    Assert(rc == 3);
    while ((rc = fscanf(file,"%f %f %f\n",&pnt.x,&pnt.y,&pnt.z)) == 3) {
      index.push_back(vertex.size()-1);
      vertex.push_back(pnt);
      segments++;
    }
    totalSegments += segments;
    fclose(file);
  }


  void parsePNTlist(const FileName &fn)
  {
    FILE *file = fopen(fn.c_str(),"r");
    Assert(file);
    for (char line[10000]; fgets(line,10000,file) && !feof(file); ) {
      char *eol = strstr(line,"\n"); if (eol) *eol = 0;
      parsePNT(line);
    }
    fclose(file);
  }

  void parseSLRAW(const FileName &fn)
  {
    FILE *file = fopen(fn.c_str(),"rb");

    if (!file) {
      cout << "WARNING: could not open file " << fn << endl;
      return;
    }

    int numStreamlines;
    int rc = fread(&numStreamlines, sizeof(int), 1, file);
    (void)rc;
    Assert(rc == 1);

    for(int s=0; s<numStreamlines; s++) {

      int numStreamlinePoints;
      rc = fread(&numStreamlinePoints, sizeof(int), 1, file);
      Assert(rc == 1);

      for(int p=0; p<numStreamlinePoints; p++) {

        vec3fa pnt;
        rc = fread(&pnt.x, 3*sizeof(float), 1, file);
        Assert(rc == 1);

        if(p != 0) index.push_back(vertex.size() - 1);
        vertex.push_back(pnt);
      }
    }
    fclose(file);
  }

  void parseSWC(const FileName &fn)
  {
    std::vector<vec3fa> filePoints;

    FILE *file = fopen(fn.c_str(),"r");
    Assert(file);
    radius = 99.f;
    for (char line[10000]; fgets(line,10000,file) && !feof(file); ) {
      if (line[0] == '#') continue;

      vec3fa v; float f1; int ID, i, j;
      sscanf(line,"%i %i %f %f %f %f %i\n",&ID,&i,&v.x,&v.y,&v.z,&f1,&j);
      if (f1 < radius)
        radius = f1;
      filePoints.push_back(v);

      index.push_back(vertex.size());
      vertex.push_back(v);
      vertex.push_back(filePoints[j-1]);
    }
    fclose(file);
  }

  void parse(const FileName &fn)
  {
    if (fn.ext() == "pnt")
      parsePNT(fn);
    else if (fn.ext() == "pntlist") {
      FILE *file = fopen(fn.c_str(),"r");
      Assert(file);
      for (char line[10000]; fgets(line,10000,file) && !feof(file); ) {
        char *eol = strstr(line,"\n"); if (eol) *eol = 0;
        parsePNT(line);
      }
      fclose(file);
    } else if (fn.ext() == "slraw") {
      parseSLRAW(fn);
    } else
      throw std::runtime_error("unknown input file format "+fn.str());
  }
  box3f getBounds() const
  {
    box3f bounds = empty;
    for (const auto &v : vertex)
      bounds.extend(v);
    return bounds;
  }
};

struct StockleyWhealCannon {
  struct Cylinder {
    vec3f v0;
    vec3f v1;
    float r;
  };
  struct Sphere {
    vec3f v;
    float r;
  };

  std::vector<Sphere> spheres[3];
  std::vector<Cylinder> cylinders[3];
  box3f bounds;

  void parse(const FileName &fn)
  {
    std::vector<vec3fa> filePoints;

    FILE *file = fopen(fn.c_str(),"r");
    Assert(file);
    bounds = empty;
    for (char line[10000]; fgets(line,10000,file) && !feof(file); ) {
      if (line[0] == '#') continue;

      vec3f v; float radius; int id, parent, type;
      sscanf(line,"%i %i %f %f %f %f %i\n",
             &id, &type, &v.x, &v.y, &v.z, &radius, &parent);
      filePoints.push_back(v);
      Assert(filePoints.size() == id); // assumes index-1==id
      bounds.extend(v); // neglects radius

      if (parent == -1) // root soma, just one sphere
        spheres[0].push_back(Sphere{v, radius});
      else { // cylinder with sphere at end
        int idx;
        switch (type) {
        case 3: idx = 1; break; // basal dendrite
        case 4: idx = 2; break; // apical dendrite
        default: idx = 0; break; // soma / axon
        }
        spheres[idx].push_back(Sphere{v, radius});
        cylinders[idx].push_back(Cylinder{filePoints[parent-1], v, radius});
      }
    }
    fclose(file);
  }

  box3f getBounds() const
  {
    return bounds;
  }
};

static const char *delim = "\n\t\r ";

void osxParseInts(std::vector<int> &vec, const std::string &content)
{
  char *s = strdup(content.c_str());
  char *tok = strtok(s,delim);
  while (tok) {
    vec.push_back(atol(tok));
    tok = strtok(NULL,delim);
  }
  free(s);
}

template<typename T> T ato(const char *);
template<> inline int ato<int>(const char *s) { return atol(s); }
template<> inline float ato<float>(const char *s) { return atof(s); }

template<typename T>
vec_t<T,3> osxParseVec3(char * &tok) {
  vec_t<T,3> v;

  assert(tok);
  v.x = ato<T>(tok);
  tok = strtok(NULL,delim);

  assert(tok);
  v.y = ato<T>(tok);
  tok = strtok(NULL,delim);

  assert(tok);
  v.z = ato<T>(tok);
  tok = strtok(NULL,delim);

  return v;
}

void osxParseVec3is(std::vector<vec3i> &vec, const std::string &content)
{
  char *s = strdup(content.c_str());
  char *tok = strtok(s,delim);
  while (tok)
    vec.push_back(osxParseVec3<int>(tok));
  free(s);
}

void osxParseVec3fas(std::vector<vec3fa> &vec, const std::string &content)
{
  char *s = strdup(content.c_str());
  char *tok = strtok(s,delim);
  while (tok)
    vec.push_back(osxParseVec3<float>(tok));
  free(s);
}

void osxParseColors(std::vector<vec4f> &vec, const std::string &content)
{
  char *s = strdup(content.c_str());
  char *tok = strtok(s,delim);
  while (tok)
    vec.push_back(vec4f(osxParseVec3<float>(tok), 1.f));
  free(s);
}

/*! parse ospray xml file */
void parseOSX(StreamLines *streamLines,
              Triangles *triangles,
              const std::string &fn)
{
  std::shared_ptr<xml::XMLDoc> doc = xml::readXML(fn);
  assert(doc);
  if (doc->child.size() != 1 || doc->child[0]->name != "OSPRay")
    throw std::runtime_error("could not parse osx file: Not in OSPRay format!?");
  const xml::Node &root_element = *doc->child[0];
  xml::for_each_child_of(root_element,[&](const xml::Node &node){
      if (node.name == "Info") {
        // ignore
      }
      else if (node.name == "Model") {
        const xml::Node &model_node = node;
        xml::for_each_child_of(model_node,[&](const xml::Node &node){
            if (node.name == "StreamLines") {
              const xml::Node &sl_node = node;
              xml::for_each_child_of(sl_node,[&](const xml::Node &node){
                  if (node.name == "vertex") {
                    osxParseVec3fas(streamLines->vertex,node.content);
                  }
                  else if (node.name == "index") {
                    osxParseInts(streamLines->index,node.content);
                  };
                });
            }
            else if (node.name == "TriangleMesh") {
              const xml::Node &tris_node = node;
              xml::for_each_child_of(tris_node,[&](const xml::Node &node){
                  if (node.name == "vertex") {
                    osxParseVec3fas(triangles->vertex,node.content);
                  }
                  else if (node.name == "color") {
                    osxParseColors(triangles->color,node.content);
                  }
                  else if (node.name == "index") {
                    osxParseVec3is(triangles->index,node.content);
                  }
                });
            }
          });
      }
    });
}

void exportOSX(const char *fn,StreamLines *streamLines, Triangles *triangles)
{
  FILE *file = fopen(fn,"w");
  fprintf(file,"<?xml version=\"1.0\"?>\n\n");
  fprintf(file,"<OSPRay>\n");
  {
    fprintf(file,"<Model>\n");
    {
      fprintf(file,"<StreamLines>\n");
      {
        fprintf(file,"<vertex>\n");
        for (const auto &v : streamLines->vertex)
          fprintf(file,"%f %f %f\n", v.x, v.y, v.z);
        fprintf(file,"</vertex>\n");

        fprintf(file,"<index>\n");
        for (const auto & i : streamLines->index)
          fprintf(file,"%i ",i);
        fprintf(file,"\n</index>\n");
      }
      fprintf(file,"</StreamLines>\n");


      fprintf(file,"<TriangleMesh>\n");
      {
        fprintf(file,"<vertex>\n");
        for (const auto &v : triangles->vertex)
          fprintf(file,"%f %f %f\n", v.x, v.y, v.z);
        fprintf(file,"</vertex>\n");

        fprintf(file,"<color>\n");
        for (const auto &c : triangles->color)
          fprintf(file,"%f %f %f\n", c.x, c.y, c.z);
        fprintf(file,"</color>\n");

        fprintf(file,"<index>\n");
        for (const auto &i : triangles->index)
          fprintf(file,"%i %i %i\n", i.x, i.y, i.z);
        fprintf(file,"</index>\n");

      }
      fprintf(file,"</TriangleMesh>\n");
    }
    fprintf(file,"</Model>\n");
  }
  fprintf(file,"</OSPRay>\n");
  fclose(file);
}

// Class definitions //////////////////////////////////////////////////////////

StreamLineSceneParser::StreamLineSceneParser(cpp::Renderer renderer) :
  renderer(renderer)
{
}

bool StreamLineSceneParser::parse(int ac, const char **&av)
{
  bool loadedScene = false;

  StreamLines *streamLines = nullptr;//new StreamLines;
  Triangles   *triangles = nullptr;//new Triangles;
  StockleyWhealCannon *swc = nullptr;//new StockleyWhealCannon;

  for (int i = 1; i < ac; i++) {
    std::string arg = av[i];
    if (arg[0] != '-') {
      const FileName fn = arg;
      if (fn.ext() == "osx") {
        streamLines = new StreamLines;
        triangles   = new Triangles;
        parseOSX(streamLines, triangles, fn);
        loadedScene = true;
      }
      else if (fn.ext() == "pnt") {
        streamLines = new StreamLines;
        streamLines->parsePNT(fn);
        loadedScene = true;
      }
      else if (fn.ext() == "swc") {
        swc = new StockleyWhealCannon;
        swc->parse(fn);
        loadedScene = true;
      }
      else if (fn.ext() == "pntlist") {
        streamLines = new StreamLines;
        streamLines->parsePNTlist(fn);
        loadedScene = true;
      }
      else if (fn.ext() == "slraw") {
        streamLines = new StreamLines;
        streamLines->parseSLRAW(fn);
        loadedScene = true;
      }
      else if (fn.ext() == "sv") {
        triangles   = new Triangles;
        triangles->parseSV(fn);
        loadedScene = true;
      }
    } else if (arg == "--streamline-radius") {
      streamLines->radius = atof(av[++i]);
#if 0
    } else if (arg == "--streamline-export") {
      exportOSX(av[++i], streamLines, triangles);
#endif
    }
  }

  if (loadedScene) {
    sceneModel = make_unique<cpp::Model>();

    auto &model = *sceneModel;

    OSPMaterial mat = ospNewMaterial(renderer.handle(), "default");
    if (mat) {
      ospSet3f(mat, "kd", .7, .7, .7);
      ospCommit(mat);
    }

    box3f bounds(empty);

    if (streamLines && !streamLines->index.empty()) {
      OSPGeometry geom = ospNewGeometry("streamlines");
      Assert(geom);
      OSPData vertex = ospNewData(streamLines->vertex.size(),
                                  OSP_FLOAT3A, &streamLines->vertex[0]);
      OSPData index  = ospNewData(streamLines->index.size(),
                                  OSP_UINT, &streamLines->index[0]);
      ospSetObject(geom,"vertex",vertex);
      ospSetObject(geom,"index",index);
      ospSet1f(geom,"radius",streamLines->radius);
      if (mat)
        ospSetMaterial(geom,mat);
      ospCommit(geom);
      ospAddGeometry(model.handle(), geom);
      bounds.extend(streamLines->getBounds());
    }

    if (triangles && !triangles->index.empty()) {
      OSPGeometry geom = ospNewGeometry("triangles");
      Assert(geom);
      OSPData vertex = ospNewData(triangles->vertex.size(),
                                  OSP_FLOAT3A, &triangles->vertex[0]);
      OSPData index  = ospNewData(triangles->index.size(),
                                  OSP_INT3, &triangles->index[0]);
      OSPData color  = ospNewData(triangles->color.size(),
                                  OSP_FLOAT4, &triangles->color[0]);
      ospSetObject(geom, "vertex", vertex);
      ospSetObject(geom, "index", index);
      ospSetObject(geom, "vertex.color", color);
      ospSetMaterial(geom, mat);
      ospCommit(geom);
      ospAddGeometry(model.handle(), geom);
      bounds.extend(triangles->getBounds());
    }

    if (swc && !swc->bounds.empty()) {
      OSPMaterial material[3];
      material[0] = mat;
      material[1] = ospNewMaterial(renderer.handle(), "default");
      if (material[1]) {
        ospSet3f(material[1], "kd", .0, .7, .0); // OBJ renderer, green
        ospCommit(material[1]);
      }
      material[2] = ospNewMaterial(renderer.handle(), "default");
      if (material[2]) {
        ospSet3f(material[2], "kd", .7, .0, .7); // OBJ renderer, magenta
        ospCommit(material[2]);
      }

      OSPGeometry spheres[3], cylinders[3];
      for (int i = 0; i < 3; i++) {
        spheres[i] = ospNewGeometry("spheres");
        Assert(spheres[i]);

        OSPData data = ospNewData(swc->spheres[i].size(), OSP_FLOAT4,
                                  &swc->spheres[i][0]);
        ospSetObject(spheres[i], "spheres", data);
        ospSet1i(spheres[i], "offset_radius", 3*sizeof(float));

        if (material[i])
          ospSetMaterial(spheres[i], material[i]);

        ospCommit(spheres[i]);
        ospAddGeometry(model.handle(), spheres[i]);

        cylinders[i] = ospNewGeometry("cylinders");
        Assert(cylinders[i]);

        data = ospNewData(swc->cylinders[i].size()*7, OSP_FLOAT,
                          &swc->cylinders[i][0]);
        ospSetObject(cylinders[i], "cylinders", data);

        if (material[i])
          ospSetMaterial(cylinders[i], material[i]);

        ospCommit(cylinders[i]);
        ospAddGeometry(model.handle(), cylinders[i]);
      }

      bounds.extend(swc->getBounds());
    }

    model.commit();
    sceneBbox = bounds;
  }

  if (streamLines) delete streamLines;
  if (triangles)   delete triangles;
  if (swc)         delete swc;

  return loadedScene;
}

std::deque<cpp::Model> StreamLineSceneParser::model() const
{
  std::deque<ospray::cpp::Model> models;
  models.push_back(sceneModel.get() == nullptr ? cpp::Model() : *sceneModel);
  return models;
}

std::deque<box3f> StreamLineSceneParser::bbox() const
{
  std::deque<ospcommon::box3f> boxes;
  boxes.push_back(sceneBbox);
  return boxes;
}
