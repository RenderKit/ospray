#undef NDEBUG

#include "hbvh.h"

namespace ospray {
  namespace hair {

  }
}

int main(int ac, char **av)
{
  std::string inFileName, outFileName; 
  if (ac == 2)
    inFileName = av[1];
  else if (ac == 3) {
    inFileName = av[1];
    outFileName = av[2];
  } else Assert2(0,"invalid num arguments: usage: hbvhbuild <in.hg> <out.hbvh>");
  ospray::hair::HairGeom hg;
  FILE *outFile = NULL;
  if (outFileName != "")
    outFile = fopen(outFileName.c_str(),"wb");

  hg.load(inFileName);
  ospray::hair::HairBVH bvh(&hg);
  bvh.build(outFile);
}
