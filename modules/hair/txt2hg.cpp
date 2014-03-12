#include "hairgeom.h"

namespace ospray {
  namespace hair {

    void usage() {
      cout << "usage: ./hair_txt2hb <infile.txt> <outfile.hb>" << endl;
      exit(0);
    }

    void txt2hb(int ac, char **av)
    {
      if (ac != 3) usage();

      std::string inFileName = av[1];      
      std::string outFileName = av[2];

      HairGeom hg;
#if 1
      hg.convertOOC(inFileName,outFileName);
#else
      hg.importTXT(inFileName);
      hg.save(outFileName);
#endif
    }
  }
}

int main(int ac, char ** av)
{
  ospray::hair::txt2hb(ac,av);
}
