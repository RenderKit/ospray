#include "miniSG.h"
#include "importer.h"

namespace ospray {
  namespace miniSG {
    using std::cout;
    using std::endl;

    struct STLTriangle {
      vec3f normal;
      vec3f v0, v1, v2;
      uint16 attribute;
    };

    /*! import a list of STL files */
    void importSTL(std::vector<Model *> &animation, const embree::FileName &fileName)
    {
      FILE *file = fopen(fileName.c_str(),"r");
      if (!file) error("could not open input file");
      char line[10000];
      while (fgets(line,10000,file) && !feof(file)) {
        char *eol = strstr(line,"\n");
        if (eol) *eol = 0;
        Model *model = new Model;
        animation.push_back(model);
        importSTL(*model,line);
      }
      cout << "done importing STL animation; found " 
           << animation.size() << " time steps" << endl;
      fclose(file);
    }

    void importSTL(Model &model,
                   const embree::FileName &fileName)
    {
      FILE *file = fopen(fileName.c_str(),"rb");
      if (!file) error("could not open input file");
      char header[80];
      int32 rc = fread(header,1,80,file);
      if (rc < 80)
        error("could not read header");
      int32 numTriangles;
      rc = fread(&numTriangles,sizeof(int),1,file);
      Assert(rc == 1 && "could not read num triangles from STL file");
      cout << "miniSG::importSTL: #tris="
           << numTriangles << " (" << fileName.c_str() << ")" << endl;

      ImportHelper importer(model,fileName.c_str());

      miniSG::Triangle triangle;
      STLTriangle stlTri;
      for (int32 i=0 ; i < numTriangles ; i++) {
        rc = fread(&stlTri.normal,sizeof(stlTri.normal),1,file);
        Assert(rc == 1 && "partial or broken STL file!?");
        rc = fread(&stlTri.v0,sizeof(stlTri.v0),1,file);
        Assert(rc == 1 && "partial or broken STL file!?");
        rc = fread(&stlTri.v1,sizeof(stlTri.v1),1,file);
        Assert(rc == 1 && "partial or broken STL file!?");
        rc = fread(&stlTri.v2,sizeof(stlTri.v2),1,file);
        Assert(rc == 1 && "partial or broken STL file!?");
        rc = fread(&stlTri.attribute,sizeof(unsigned short),1,file);
        Assert(rc == 1 && "partial or broken STL file!?");
        
        triangle.v0 = importer.addVertex(stlTri.v0);
        triangle.v1 = importer.addVertex(stlTri.v1);
        triangle.v2 = importer.addVertex(stlTri.v2);
        triangle.materialID = -1;
        importer.addTriangle(triangle);
      }
      importer.finalize();
    }
  }
}


