#include "mpi/mpicommon.h"
#include "ospray/ospray.h"
#include "common/ospcommon.h"

int main(int ac, const char **av)
{
  ospInit(&ac,av);
  ospLoadModule("mpiTest");
  std::cout << "done mpitest" << std::endl;
}


