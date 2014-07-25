#include "lammpsModel.h"
#include "loc.h"

extern int  yyparse();
extern void yyerror(const char* msg);
extern FILE *yyin;
extern int yydebug;

namespace ospray {
  namespace lammps {
    
    Model *parserModel = NULL;

  inline vec3f makeRandomColor(const int i)
  {
    const int mx = 13*17*43;
    const int my = 11*29;
    const int mz = 7*23*63;
    const uint32 g = (i * (3*5*127)+12312314);
    return vec3f((g % mx)*(1.f/(mx-1)),
                 (g % my)*(1.f/(my-1)),
                 (g % mz)*(1.f/(mz-1)));
  }

    int Model::getAtomType(const std::string &name)
    {
      if (atomTypeByName.find(name) == atomTypeByName.end()) {
        std::cout << "Found atom type '"+name+"'" << std::endl;
        AtomType *a = new AtomType(name);
        a->color = makeRandomColor(atomType.size());
        atomTypeByName[name] = atomType.size();
        atomType.push_back(a);
      }
      return atomTypeByName[name];
    }

    void Model::load_DAT_XYZ(const std::string &fileName)
    {
      FILE *file = fopen(fileName.c_str(),"r");
      if (!file) 
        throw std::runtime_error("could not open input file "+fileName);
      int numAtoms;
      int rc = fscanf(file,"%i\nDAT\n",&numAtoms);
      if (rc != 1)
        throw std::runtime_error("could not parse .dat.xyz header in input file "+fileName);

      std::cout << "#" << fileName << " (.dat.xyz format): expecting " << numAtoms << " atoms" << std::endl;
      for (int i=0;i<numAtoms;i++) {
        char atomName[110];
        Atom a;
        rc = fscanf(file,"%100s %f %f %f\n",atomName,&a.position.x,&a.position.y,&a.position.z);
        if (rc != 4) {
          std::stringstream ss;
          ss << "in " << fileName << " (line " << (i+2) << "): "
             << "could not parse .dat.xyz data line" << std::endl;
          throw std::runtime_error(ss.str());
        }
        a.type = getAtomType(atomName);
        atom.push_back(a);
      }
    }

    void Model::load_LAMMPS_XYZ(const std::string &fileName)
    {
      parserModel = this;

      yydebug = 0;

      Loc::current.name = fileName.c_str();
      Loc::current.line = 1;
      std::cout << "#osp:lammpsView: --- parsing " << fileName << " ---" << std::endl;
      yyin=fopen(fileName.c_str(),"r"); //popen(cmd,"r");
      if (!yyin)
        Error(Loc::current,"#osp:lammpsView: can't open file...");

      yyparse();
      fclose(yyin);
    }

    box3f Model::getBBox() const 
    {
      box3f bbox = embree::empty;
      for (int i=0;i<atom.size();i++)
        bbox.extend(atom[i].position);
      return bbox;
    }

  }
}


