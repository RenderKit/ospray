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
  std::vector<vec3fa> color; // vertex color, from sv's 'v' value
  std::vector<vec3i>  index;

  struct SVVertex {
    float v;
    vec3f pos; //float x,y,z;
  };

  struct SVTriangle {
    SVVertex vertex[3];
  };

  vec3f lerpf(float x, float x0,float x1,vec3f y0, vec3f y1)
  {
    float f = (x-x0)/(x1-x0);
    return f*y1+(1-f)*y0;
  }
  vec3f colorOf(const float f)
  {
    if (f < .5f)
      return vec3f(lerpf(f, 0.f,.5f,vec3f(0),vec3f(0,1,0)));
    else
      return vec3f(lerpf(f, .5f,1.f,vec3f(0,1,0),vec3f(1,0,0)));
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
    for (int i=0;i<vertex.size();i++)
      bounds.extend(vertex[i]);
    return bounds;
  }
};

struct StreamLines {
  std::vector<vec3fa> vertex;
  std::vector<int>    index;
  float radius;

  StreamLines() : radius(0.001f) {}

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
    for (int i=0;i<vertex.size();i++)
      bounds.extend(vertex[i]);
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
        spheres[0].push_back((Sphere){v, radius});
      else { // cylinder with sphere at end
        int idx;
        switch (type) {
          case 3: idx = 1; break; // basal dendrite
          case 4: idx = 2; break; // apical dendrite
          default: idx = 0; break; // soma / axon
        }
        spheres[idx].push_back((Sphere){v, radius});
        cylinders[idx].push_back((Cylinder){filePoints[parent-1], v, radius});
      }
    }
    fclose(file);
  }

  box3f getBounds() const
  {
    return bounds;
  }
};

void osxParseInts(std::vector<int> &vec, const std::string &content)
{
  char *s = strdup(content.c_str());
  const char *delim = "\n\t\r ";
  char *tok = strtok(s,delim);
  while (tok) {
    vec.push_back(atol(tok));
    tok = strtok(NULL,delim);
  }
  free(s);
}

void osxParseVec3is(std::vector<vec3i> &vec, const std::string &content)
{
  char *s = strdup(content.c_str());
  const char *delim = "\n\t\r ";
  char *tok = strtok(s,delim);
  while (tok) {
    vec3i v;
    assert(tok);
    v.x = atol(tok);
    tok = strtok(NULL,delim);

    assert(tok);
    v.y = atol(tok);
    tok = strtok(NULL,delim);

    assert(tok);
    v.z = atol(tok);
    tok = strtok(NULL,delim);

    vec.push_back(v);
  }
  free(s);
}

void osxParseVec3fas(std::vector<vec3fa> &vec, const std::string &content)
{
  char *s = strdup(content.c_str());
  const char *delim = "\n\t\r ";
  char *tok = strtok(s,delim);
  while (tok) {
    vec3fa v;
    assert(tok);
    v.x = atof(tok);
    tok = strtok(NULL,delim);

    assert(tok);
    v.y = atof(tok);
    tok = strtok(NULL,delim);

    assert(tok);
    v.z = atof(tok);
    tok = strtok(NULL,delim);

    vec.push_back(v);
  }
  free(s);
}

/*! parse ospray xml file */
void parseOSX(StreamLines *streamLines,
              Triangles *triangles,
              const std::string &fn)
{
  xml::XMLDoc *doc = xml::readXML(fn);
  assert(doc);
  if (doc->child.size() != 1 || doc->child[0]->name != "OSPRay")
    throw std::runtime_error("could not parse osx file: Not in OSPRay format!?");
  xml::Node *root_element = doc->child[0];
  for (int childID=0;childID<root_element->child.size();childID++) {
    xml::Node *node = root_element->child[childID];
    if (node->name == "Info") {
      // ignore
      continue;
    }

    if (node->name == "Model") {
      xml::Node *model_node = node;
      for (int childID=0;childID<model_node->child.size();childID++) {
        xml::Node *node = model_node->child[childID];

        if (node->name == "StreamLines") {

          xml::Node *sl_node = node;
          for (int childID=0;childID<sl_node->child.size();childID++) {
            xml::Node *node = sl_node->child[childID];
            if (node->name == "vertex") {
              osxParseVec3fas(streamLines->vertex,node->content);
              continue;
            };
            if (node->name == "index") {
              osxParseInts(streamLines->index,node->content);
              continue;
            };
          }
          continue;
        }

        if (node->name == "TriangleMesh") {
          xml::Node *tris_node = node;
          for (int childID=0;childID<tris_node->child.size();childID++) {
            xml::Node *node = tris_node->child[childID];
            if (node->name == "vertex") {
              osxParseVec3fas(triangles->vertex,node->content);
              continue;
            };
            if (node->name == "color") {
              osxParseVec3fas(triangles->color,node->content);
              continue;
            };
            if (node->name == "index") {
              osxParseVec3is(triangles->index,node->content);
              continue;
            };
          }
          continue;
        }
      }
    }
  }
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
        for (int i=0;i<streamLines->vertex.size();i++)
          fprintf(file,"%f %f %f\n",
                  streamLines->vertex[i].x,
                  streamLines->vertex[i].y,
                  streamLines->vertex[i].z);
        fprintf(file,"</vertex>\n");

        fprintf(file,"<index>\n");
        for (int i=0;i<streamLines->index.size();i++)
          fprintf(file,"%i ",streamLines->index[i]);
        fprintf(file,"\n</index>\n");
      }
      fprintf(file,"</StreamLines>\n");


      fprintf(file,"<TriangleMesh>\n");
      {
        fprintf(file,"<vertex>\n");
        for (int i=0;i<triangles->vertex.size();i++)
          fprintf(file,"%f %f %f\n",
                  triangles->vertex[i].x,
                  triangles->vertex[i].y,
                  triangles->vertex[i].z);
        fprintf(file,"</vertex>\n");

        fprintf(file,"<color>\n");
        for (int i=0;i<triangles->color.size();i++)
          fprintf(file,"%f %f %f\n",
                  triangles->color[i].x,
                  triangles->color[i].y,
                  triangles->color[i].z);
        fprintf(file,"</color>\n");

        fprintf(file,"<index>\n");
        for (int i=0;i<triangles->index.size();i++)
          fprintf(file,"%i %i %i\n",
                  triangles->index[i].x,
                  triangles->index[i].y,
                  triangles->index[i].z);
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
  m_renderer(renderer)
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
#if 1
    }
#else
    } else if (arg == "--streamline-export") {
      exportOSX(av[++i], streamLines, triangles);
    }
#endif
  }

  if (loadedScene) {
    m_model = ospNewModel();

    OSPMaterial mat = ospNewMaterial(m_renderer.handle(), "default");
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
      ospAddGeometry(m_model.handle(), geom);
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
                                  OSP_FLOAT3A, &triangles->color[0]);
      ospSetObject(geom, "vertex", vertex);
      ospSetObject(geom, "index", index);
      ospSetObject(geom, "vertex.color", color);
      ospSetMaterial(geom, mat);
      ospCommit(geom);
      ospAddGeometry(m_model.handle(), geom);
      bounds.extend(triangles->getBounds());
    }

    if (swc && !swc->bounds.empty()) {
      OSPMaterial material[3];
      material[0] = mat;
      material[1] = ospNewMaterial(m_renderer.handle(), "default");
      if (material[1]) {
        ospSet3f(material[1], "kd", .0, .7, .0); // OBJ renderer, green
        ospCommit(material[1]);
      }
      material[2] = ospNewMaterial(m_renderer.handle(), "default");
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
        ospAddGeometry(m_model.handle(), spheres[i]);


        cylinders[i] = ospNewGeometry("cylinders");
        Assert(cylinders[i]);

        data = ospNewData(swc->cylinders[i].size()*7, OSP_FLOAT,
                          &swc->cylinders[i][0]);
        ospSetObject(cylinders[i], "cylinders", data);

        if (material[i])
          ospSetMaterial(cylinders[i], material[i]);

        ospCommit(cylinders[i]);
        ospAddGeometry(m_model.handle(), cylinders[i]);
      }

      bounds.extend(swc->getBounds());
    }

    m_model.commit();
    m_bbox = bounds;
  }

  if (streamLines) delete streamLines;
  if (triangles)   delete triangles;
  if (swc)         delete swc;

  return loadedScene;
}

cpp::Model StreamLineSceneParser::model() const
{
  return m_model;
}

box3f StreamLineSceneParser::bbox() const
{
  return m_bbox;
}
