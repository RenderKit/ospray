#undef NDEBUG

#include "hkd.h"

namespace ospray {
  namespace hair {

  }
}

int main(int ac, char **av)
{
  Assert(ac == 2);
  std::string inFileName = av[1];
  ospray::hair::HairGeom hg;
  hg.load(inFileName);
  ospray::hair::HairKD kd(&hg);
  kd.build();
}
