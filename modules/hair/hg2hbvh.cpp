#undef NDEBUG

#include "hbvh.h"

namespace ospray {
  namespace hair {
    extern float hairlet_threshold_in_max_radius;
  }
}

void usage(const std::string &err = "")
{
  if (err != "") std::cout << "error : " << err << std::endl;
  std::cout << "./hg2hbvh inFile.hg -o outFile.hg [parms]" << std::endl;
  std::cout << " --rad-threshold <subdiv-threshold>" << std::endl;
  exit(0);
}

int main(int ac, char **av)
{
  std::string inFileName,outFileName; 

  for (int i=1;i<ac;i++) {
    std::string arg = av[i];
    if (arg == "--rad-threshold") {
      ospray::hair::hairlet_threshold_in_max_radius = atoi(av[++i]);
    } else if (arg == "-o") {
      outFileName = av[++i];
    } else if (arg[0] == '-') 
      usage("unkonwn arg "+arg);
    else {
      if (inFileName != "") usage();
      inFileName = arg;
    }
  }
  if (inFileName == "") usage("no input file specified");
  if (outFileName == "") usage("no output file specified");
  // if (ac == 2)
  //   inFileName = av[1];
  // else if (ac == 3) {
  //   inFileName = av[1];
  //   outFileName = av[2];
  // } else Assert2(0,"invalid num arguments: usage: hbvhbuild <in.hg> <out.hbvh>");
  ospray::hair::HairGeom hg;
  FILE *outFile = NULL;
  if (outFileName != "")
    outFile = fopen(outFileName.c_str(),"wb");

  hg.load(inFileName);
  ospray::hair::HairBVH bvh(&hg);
  bvh.build(outFile);
}
