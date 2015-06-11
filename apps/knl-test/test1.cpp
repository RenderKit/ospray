#include "ospray/common/OSPCommon.h"

namespace ospray {
  using namespace std;

  volatile float ospZero = 0.f;

  void runScalar(int num_runs)
  {
    cout << "simple test of scalar arithmetic, using " << num_runs << " runs" << endl;
    double time_begin = getSysTime();

    float t = 1.f;
    for (size_t i=0;i<num_runs*1000000LL;i++) {
      t = t + expf(t*logf(t)*1e-20f);
      t = t + expf(t*logf(t)*1e-22f);
      t = t + expf(t*logf(t)*1e-24f);
    }

    double time_end = getSysTime();
    PRINT(t);
    cout << "time to run " << (time_end-time_begin) << " seconds" << endl;
  }

  void runTest(int ac, char **av)
  {
    const int num_runs = ac > 1 ? atoi(av[1]) : 100;
    void runScalar(num_runs);
  }
}

int main(int ac, char **av)
{
  ospray::runTest(ac,av);
}
