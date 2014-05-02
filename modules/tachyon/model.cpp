#include "model.h"

namespace ospray {
  namespace tachyon {

    using std::cout;
    using std::endl;

    const char *TOK = "\t\n ";
    const int LINESZ=10000;

    Model::Model()
      : bounds(embree::empty),
        resolution(-1)
    {
    }

    box3f Model::getBounds() const { return bounds; }

    bool Model::empty() const { return bounds.empty(); }

    int Model::addTexture(const Texture &texture)
    {
      // ignoring textures for now
      return -1;
    }
    void Model::addTriangle(const Triangle &triangle)
    {
      this->triangle.push_back(triangle);
      bounds.extend(triangle.v0);
      bounds.extend(triangle.v1);
      bounds.extend(triangle.v2);
    }
    void Model::addSphere(const Sphere &sphere)
    {
      this->sphere.push_back(sphere);
      bounds.extend(sphere.center - sphere.rad);
      bounds.extend(sphere.center + sphere.rad);
    }
    void Model::addCylinder(const Cylinder &cylinder)
    {
      this->cylinder.push_back(cylinder);
      bounds.extend(cylinder.base + cylinder.rad);
      bounds.extend(cylinder.base - cylinder.rad);
      bounds.extend(cylinder.apex + cylinder.rad);
      bounds.extend(cylinder.apex - cylinder.rad);
    }


    int parseTexture(Model &model, FILE *file)
    {
      tachyon::Texture texture;
      int rc;
      char line[LINESZ+1];
      fgets(line,LINESZ,file); // Texture
      fgets(line,LINESZ,file); //   ambient
      rc = sscanf(line,"  Ambient %f Diffuse %f Specular %f Opacity %f",
                  &texture.ambient,
                  &texture.diffuse,
                  &texture.specular,
                  &texture.opacity);
      if (rc != 4)
        error("cannot parse texture line "+std::string(line));

      fgets(line,LINESZ,file); //   phong
      rc = sscanf(line,"  Phong Plastic %f Phong_size %f Color %f %f %f TexFunc %i",
                  &texture.phong.plastic,
                  &texture.phong.size,
                  &texture.phong.color.x,
                  &texture.phong.color.y,
                  &texture.phong.color.z,
                  &texture.phong.texFunc);
      if (rc != 6)
        error("cannot parse texture line "+std::string(line));
      return model.addTexture(texture);
    }
    void parseFCylinder(tachyon::Model &model, FILE *file)
    {
      int rc;
      char line[LINESZ+1];

      Cylinder cylinder;
      fgets(line,LINESZ,file); // Texture
      rc = sscanf(line,"  Base %f %f %f\n",
                  &cylinder.base.x,
                  &cylinder.base.y,
                  &cylinder.base.z);
      if (rc != 3)
        error("cannot parse cylinder line "+std::string(line));

      fgets(line,LINESZ,file); // Texture
      rc = sscanf(line,"  Apex %f %f %f",
                  &cylinder.apex.x,
                  &cylinder.apex.y,
                  &cylinder.apex.z);
      if (rc != 3)
        error("cannot parse cylinder line "+std::string(line));

      fgets(line,LINESZ,file); // Texture
      rc = sscanf(line,"  Rad %f",
                  &cylinder.rad);
      if (rc != 1)
        error("cannot parse cylinder line "+std::string(line));
      
      cylinder.textureID = parseTexture(model,file);
      model.addCylinder(cylinder);
      // Texture texture;
      // parseTexture(texture,file);
      // model.cylinder.push_back(cylinder);
    }
    void parseSphere(tachyon::Model &model, FILE *file)
    {
      int rc;
      char line[LINESZ+1];

      Sphere sphere;
      fgets(line,LINESZ,file); // Texture
      rc = sscanf(line,"  Center %f %f %f\n",
                  &sphere.center.x,
                  &sphere.center.y,
                  &sphere.center.z);
      if (rc != 3)
        error("cannot parse sphere line "+std::string(line));

      fgets(line,LINESZ,file); // Texture
      rc = sscanf(line,"  Rad %f",
                  &sphere.rad);
      if (rc != 1)
        error("cannot parse sphere line "+std::string(line));
      
      sphere.textureID = parseTexture(model,file);
      model.addSphere(sphere);
      // Texture texture;
      // parseTexture(texture,file);
      // model.sphere.push_back(sphere);
    }
    void parseSTri(tachyon::Model &model, FILE *file)
    {
      int rc;
      char line[LINESZ+1];

      Triangle triangle;

      fgets(line,LINESZ,file);
      rc = sscanf(line,"  V0 %f %f %f\n",&triangle.v0.x,&triangle.v0.y,&triangle.v0.z);
      if (rc != 3)
        error("cannot parse stri line "+std::string(line));

      fgets(line,LINESZ,file);
      rc = sscanf(line,"  V1 %f %f %f\n",&triangle.v1.x,&triangle.v1.y,&triangle.v1.z);
      if (rc != 3)
        error("cannot parse stri line "+std::string(line));

      fgets(line,LINESZ,file);
      rc = sscanf(line,"  V2 %f %f %f\n",&triangle.v2.x,&triangle.v2.y,&triangle.v2.z);
      if (rc != 3)
        error("cannot parse stri line "+std::string(line));

      fgets(line,LINESZ,file);
      rc = sscanf(line,"  N0 %f %f %f\n",&triangle.n0.x,&triangle.n0.y,&triangle.n0.z);
      if (rc != 3)
        error("cannot parse stri line "+std::string(line));

      fgets(line,LINESZ,file);
      rc = sscanf(line,"  N1 %f %f %f\n",&triangle.n1.x,&triangle.n1.y,&triangle.n1.z);
      if (rc != 3)
        error("cannot parse stri line "+std::string(line));

      fgets(line,LINESZ,file);
      rc = sscanf(line,"  N2 %f %f %f\n",&triangle.n2.x,&triangle.n2.y,&triangle.n2.z);
      if (rc != 3)
        error("cannot parse stri line "+std::string(line));
      
      triangle.textureID = parseTexture(model,file);
      model.addTriangle(triangle);
      // Texture texture;
      // parseTexture(texture,file);
      // model.triangle.push_back(tri);
    }

    void importFile(tachyon::Model &model, const std::string &fileName)
    {
      FILE *file = fopen(fileName.c_str(),"r");
      if (!file) error("could not open '"+fileName+"'");
      char line[LINESZ+1];
      while(fgets(line,LINESZ,file) && !feof(file)) {
        // remove eol indicator, if present
        char *eol = strstr(line,"\n");
        if (eol) *eol = 0;
      
        if (line[0] == 0) 
          // empty
          continue;
        if (line[0] == '#') 
          // comment
          continue; 
        std::string token = strtok(line,TOK);
        if (token == "Begin_Scene") {
          // ignore
          continue;
        } 
        if (token == "End_Scene") {
          // ignore
          break;
        } 
        if (token == "Resolution") {
          model.resolution.x = atoi(strtok(NULL,TOK));
          model.resolution.y = atoi(strtok(NULL,TOK));
          continue;
        }
        if (token == "Shader_Mode") {
          // ignore
          while (fgets(line,LINESZ,file) && !feof(file)) {
            cout << "shade_mode ignoring " << line;
            std::string token2 = strtok(line,TOK);
            if (token2 == "End_Shader_Mode") break;
          }
          continue;
        }
        if (token == "Camera") {
          // ignore
          while (fgets(line,LINESZ,file) && !feof(file)) {
            cout << "camera ignoring " << line;
            std::string token2 = strtok(line,TOK);
            if (token2 == "End_Camera") break;
          }
          continue;
        }
        if (token == "Directional_Light") {
          cout << "ignoring Directional_Light" << endl;
          continue;
        }
        if (token == "Background") {
          cout << "ignoring Background" << endl;
          continue;
        }
        if (token == "Fog") {
          cout << "ignoring Fog" << endl;
          continue;
        }
        if (token == "FCylinder") {
          parseFCylinder(model,file);
          continue;
        }
        if (token == "Sphere") {
          parseSphere(model,file);
          continue;
        }
        if (token == "STri") {
          parseSTri(model,file);
          continue;
        }
        error("unsupported tachyon command '"+token+"'");
      }
      fclose(file);
      cout << "#osp::tachyon: done parsing tachyon file" << endl;
      cout << "#osp::tachyon:   found " << model.numTriangles() << " triangles" << endl;
      cout << "#osp::tachyon:   found " << model.numCylinders() << " cylinders" << endl;
      cout << "#osp::tachyon:   found " << model.numSpheres() << " spheres" << endl;
    }
  }
}
