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

// ospray
#include "common/AffineSpace.h"
// tachyon module
#include "Model.h"
#include "Loc.h"
// std
#include <fstream>

// flex/bison stuff
extern int yydebug;
extern FILE *yyin;
extern int yyparse();

namespace ospray {
  namespace tachyon {

    Model *parserModel = NULL;

    using std::cout;
    using std::endl;

    const char *TOK = "\t\n ";
    const int LINESZ=10000;

    // number of trianlges exported to embree (if export enabled)
    long numExported = 0;

    inline bool operator<(const Texture &a, const Texture &b)
    {
      int v = memcmp(&a,&b,sizeof(a));
      return v < 0;
    }
    inline bool operator==(const Texture &a, const Texture &b)
    {
      int v = memcmp(&a,&b,sizeof(a));
      return v == 0;
    }

    void importFile(tachyon::Model &model, const std::string &fileName)
    {
      yydebug = 0;
      parserModel = &model;
      Loc::current.name = fileName.c_str();
      Loc::current.line = 1;
      std::cout << "#osp:tachyon: --- parsing " << fileName << " ---" << std::endl;
      yyin=fopen(fileName.c_str(),"r"); //popen(cmd,"r");
      if (!yyin)
        Error(Loc::current,"#osp:tachyon: can't open file...");

      yyparse();
      fclose(yyin);
    }

  Texture::Texture()
      : ambient(0), diffuse(.8), specular(0), opacity(1), texFunc(0),
        color(1,1,1)
    {
    }
    Texture::~Texture()
    {
    }

    Phong::Phong()
      : plastic(0), size(0)
    {}

    Camera::Camera()
      : center(0,0,0),
        upDir(0,1,0),
        viewDir(0,0,1)
    {
    }

    Model::Model()
      : bounds(ospcommon::empty),
        camera(NULL),
        smoothTrisVA(NULL),
        backgroundColor(.1f),
        resolution(-1)
    {
    }

    box3f Model::getBounds() const { return bounds; }

    bool Model::empty() const { return bounds.empty(); }

    VertexArray *Model::getSTriVA(bool create) {
      if (!smoothTrisVA && create) {
        smoothTrisVA = new VertexArray();
        vertexArrayVec.push_back(smoothTrisVA);
      }
      return smoothTrisVA;
    }

    void Model::addTriangle(const Triangle &triangle)
    {
      this->triangleVec.push_back(triangle);
      bounds.extend(triangle.v0);
      bounds.extend(triangle.v1);
      bounds.extend(triangle.v2);
    }
    void Model::addVertexArray(VertexArray *va)
    {
      this->vertexArrayVec.push_back(va);
      for (int i=0;i<va->coord.size();i++)
        bounds.extend(va->coord[i]);
    }
    void Model::addSphere(const Sphere &sphere)
    {
      this->sphereVec.push_back(sphere);
      bounds.extend(sphere.center - vec3f(sphere.rad));
      bounds.extend(sphere.center + vec3f(sphere.rad));
    }
    void Model::addDirLight(const DirLight &dirLight)
    {
      this->dirLightVec.push_back(dirLight);
    }
    void Model::addPointLight(const PointLight &pointLight)
    {
      this->pointLightVec.push_back(pointLight);
    }
    void Model::addCylinder(const Cylinder &cylinder)
    {
      this->cylinderVec.push_back(cylinder);
      bounds.extend(cylinder.base + vec3f(cylinder.rad));
      bounds.extend(cylinder.base - vec3f(cylinder.rad));
      bounds.extend(cylinder.apex + vec3f(cylinder.rad));
      bounds.extend(cylinder.apex - vec3f(cylinder.rad));
    }
    int Model::addTexture(Texture *texture)
    {
      assert(texture);
      for (int i=textureVec.size()-1;i>=0;--i)
        if (*texture == textureVec[i])
          return i;
      textureVec.push_back(*texture);
      int ID = textureVec.size()-1;
      // cout << "New texture[" << ID << " " << texture->color << endl;
      return ID;
    }


    void exportArray(std::ofstream &xml, FILE *bin,
                     Model &model, VertexArray *va)
    {
      char name[1000];

      int textureID = va->textureID;
      const Texture *texture = model.getTexture(textureID);

      // sprintf(name,"vertexArray_%i",ID);
      // xml << "  <TriangleMesh name=\"" << name << "\" id=\"" << ID << "\">" << endl;
      xml << "  <TriangleMesh>" << endl;
      xml << "    <environment>0</environment>" << endl;

#if 1
      vec3f Kd = texture->color * texture->diffuse;
      vec3f Ks = vec3f(texture->specular);
      float Ns = texture->phong.size;
      xml << "    <material>" << endl;
      xml << "    <code>\"OBJ\"</code>" << endl;
      xml << "    <parameters>" << endl;
      xml << "      <float name=\"d\">" << texture->opacity << "</float>" << endl;
      xml << "      <float3 name=\"Kd\">" << Kd.x << " " << Kd.y << " " << Kd.z << "</float3>" << endl;
      xml << "      <float3 name=\"Ks\">" << Ks.x << " " << Ks.y << " " << Ks.z << "</float3>" << endl;
      xml << "      <float name=\"Ns\">" << Ns << "</float>" << endl;
      xml << "    </parameters>" << endl;
      xml << "    </material>" << endl;
#else
      xml << "    <code>\"MetallicPaint\"</code>" << endl;
      xml << "    <parameters>" << endl;
      xml << "      <float name=\"eta\">1.45</float>" << endl;
      xml << "      <float3 name=\"glitterColor\">0.5 0.44 0.42</float3>" << endl;
      xml << "      <float name=\"glitterSpread\">0.01</float>" << endl;
      xml << "      <float3 name=\"shadeColor\">0.5 0.42 0.35</float3>" << endl;
      xml << "    </parameters>" << endl;
      xml << "    </material>" << endl;
#endif

      if (va->coord.size()) {
        long ofs = ftell(bin);
        for (int i=0;i<va->coord.size();i++)
          fwrite(&va->coord[i],1,sizeof(vec3f),bin);
        xml << "    <positions "
            << "ofs=\"" << ofs << "\" "
            << "size=\"" << va->coord.size() << "\"/>" << endl;
      }

      if (va->normal.size()) {
        long ofs = ftell(bin);
        for (int i=0;i<va->normal.size();i++)
          fwrite(&va->normal[i],1,sizeof(vec3f),bin);
        xml << "    <normals "
            << "ofs=\"" << ofs << "\" "
            << "size=\"" << va->normal.size() << "\"/>" << endl;
      }

      if (va->triangle.size()) {
        long ofs = ftell(bin);
        fwrite(&va->triangle[0],va->triangle.size(),sizeof(vec3i),bin);
        xml << "    <triangles "
            << "ofs=\"" << ofs << "\" "
            << "size=\"" << va->triangle.size() << "\"/>" << endl;
      }
      xml << "  </TriangleMesh>" << endl;

      numExported += va->triangle.size();
    }

    void tessellateSphereOctant(VertexArray *va,Sphere &sphere,
                                const vec3f _du, const vec3f _dv, const vec3f _dw,
                                int depth=2)
    {
      const vec3f du  = _du;              // 0
      const vec3f dv  = _dv;              // 1
      const vec3f dw  = _dw;              // 2
      const vec3f duv = normalize(du+dv); // 3
      const vec3f duw = normalize(du+dw); // 4
      const vec3f dvw = normalize(dv+dw); // 5

      if (depth > 1) {
        tessellateSphereOctant(va,sphere,du,duv,duw,depth-1);
        tessellateSphereOctant(va,sphere,dv,dvw,duv,depth-1);
        tessellateSphereOctant(va,sphere,dw,dvw,duw,depth-1);
        tessellateSphereOctant(va,sphere,duv,dvw,duw,depth-1);
      } else {
        vec3i base = vec3i(va->coord.size());
        va->coord.push_back(sphere.center+sphere.rad*du);
        va->coord.push_back(sphere.center+sphere.rad*dv);
        va->coord.push_back(sphere.center+sphere.rad*dw);
        va->normal.push_back(du);
        va->normal.push_back(dv);
        va->normal.push_back(dw);

        va->coord.push_back(sphere.center+sphere.rad*duv);
        va->coord.push_back(sphere.center+sphere.rad*duw);
        va->coord.push_back(sphere.center+sphere.rad*dvw);
        va->normal.push_back(duv);
        va->normal.push_back(duw);
        va->normal.push_back(dvw);

        va->triangle.push_back(base+vec3i(0,3,4));
        va->triangle.push_back(base+vec3i(1,3,5));
        va->triangle.push_back(base+vec3i(2,4,5));
        va->triangle.push_back(base+vec3i(3,4,5));
      }
    }

    void tessellateSphere(VertexArray *va,Sphere &sphere)
    {
      tessellateSphereOctant(va,sphere,vec3f(+1,0,0),vec3f(0,+1,0),vec3f(0,0,+1));
      tessellateSphereOctant(va,sphere,vec3f(+1,0,0),vec3f(0,+1,0),vec3f(0,0,-1));
      tessellateSphereOctant(va,sphere,vec3f(+1,0,0),vec3f(0,-1,0),vec3f(0,0,+1));
      tessellateSphereOctant(va,sphere,vec3f(+1,0,0),vec3f(0,-1,0),vec3f(0,0,-1));
      tessellateSphereOctant(va,sphere,vec3f(-1,0,0),vec3f(0,+1,0),vec3f(0,0,+1));
      tessellateSphereOctant(va,sphere,vec3f(-1,0,0),vec3f(0,+1,0),vec3f(0,0,-1));
      tessellateSphereOctant(va,sphere,vec3f(-1,0,0),vec3f(0,-1,0),vec3f(0,0,+1));
      tessellateSphereOctant(va,sphere,vec3f(-1,0,0),vec3f(0,-1,0),vec3f(0,0,-1));
    }
    void exportSpheres(std::ofstream &xml, FILE *bin,
                       Model &model, std::vector<Sphere> sphereVec)
    {
      while (sphereVec.size()) {
        int textureID = sphereVec[0].textureID;
        std::vector<Sphere> notMatching;

        VertexArray va;
        va.textureID = textureID;

        for (int i=0;i<sphereVec.size();i++) {
          if (sphereVec[i].textureID != textureID) {
            notMatching.push_back(sphereVec[i]);
            continue;
          }
          tessellateSphere(&va,sphereVec[i]);
        }

        exportArray(xml,bin,model,&va);

        sphereVec = notMatching;
        continue;
      }
    }




    void tessellateCylinder(VertexArray *va,Cylinder &cylinder)
    {
      vec3f dw = cylinder.apex - cylinder.base;
      float l = length(dw);

      AffineSpace3f space;
      space.l = frame(normalize(dw));
      space.p = cylinder.base;

      vec3i base(va->coord.size());
      int N = 16;
      for (int i=0;i<N;i++) {
        float u = cylinder.rad*cosf(i*2.f*M_PI/N);
        float v = cylinder.rad*sinf(i*2.f*M_PI/N);
        vec3f orgV0(u,v,0);
        vec3f orgV1(u,v,l);
        vec3f resV0 = xfmPoint(space,orgV0);
        vec3f resV1 = xfmPoint(space,orgV1);
        vec3f orgN(u,v,0);
        vec3f resN = xfmVector(space,orgN);
        va->coord.push_back(resV0);
        va->coord.push_back(resV1);
        va->normal.push_back(resN);
        va->normal.push_back(resN);
        int I = (2*i+0) % (2*N);
        int J = (2*i+1) % (2*N);
        int K = (2*i+2) % (2*N);
        int L = (2*i+3) % (2*N);
        va->triangle.push_back(base+vec3i(I,J,K));
        va->triangle.push_back(base+vec3i(J,K,L));
      }
    }
    void exportCylinders(std::ofstream &xml, FILE *bin,
                       Model &model, std::vector<Cylinder> cylinderVec)
    {
      while (cylinderVec.size()) {
        int textureID = cylinderVec[0].textureID;
        std::vector<Cylinder> notMatching;

        VertexArray va;
        va.textureID = textureID;

        for (int i=0;i<cylinderVec.size();i++) {
          if (cylinderVec[i].textureID != textureID) {
            notMatching.push_back(cylinderVec[i]);
            continue;
          }
          tessellateCylinder(&va,cylinderVec[i]);
        }

        exportArray(xml,bin,model,&va);

        cylinderVec = notMatching;
        continue;
      }
    }

    void Model::exportToEmbree(const std::string &fileBase)
    {
      PRINT(bounds);
      cout << "=======================================================" << endl;
      std::string xmlFileName = fileBase+".xml";
      std::string binFileName = fileBase+".bin";
      cout << "exporting to (embree-format) " << xmlFileName << "/" << binFileName << endl;

      FILE *bin = fopen(binFileName.c_str(),"wb");
      std::ofstream xml(xmlFileName.c_str());

      xml << "<?xml version=\"1.0\"?>" << endl;
      xml << "<scene>" << endl;
      xml << " <Group>" << endl;
      {
        cout << "exporting vertex arrays:";
        for (int vaID=0;vaID<vertexArrayVec.size();vaID++) {
          cout << "[" << vaID << "]" << std::flush;
          exportArray(xml,bin,*this,vertexArrayVec[vaID]);
        }
        cout << endl;
      }
      {
        cout << "exporting spheres (by material)...";
        exportSpheres(xml,bin,*this,sphereVec);
        cout << endl;
      }
      {
        cout << "exporting cylinders (by material)...";
        exportCylinders(xml,bin,*this,cylinderVec);
        cout << endl;
      }
      xml << " </Group>" << endl;
      xml << "</scene>" << endl;

      cout << "done exporting to embree format" << endl;
      cout << "exported a total of " << numExported << " triangles" << endl;
      cout << "=======================================================" << endl;
      fclose(bin);
      exit(0);
    }
  }
}
