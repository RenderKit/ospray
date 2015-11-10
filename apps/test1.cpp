#include "ospray/common/OSPCommon.h"
#include "ospray/common/Thread.h"
#include "common/sys/sync/barrier.h"
#include <sched.h>

namespace ospray {

  using namespace std;

  volatile size_t sum = 0;
  volatile int nextFreeID = 0;
  volatile bool done = 0;
  double timeToRun = 1.f;
  embree::MutexSys mutex;
  embree::BarrierSys barrier;

  struct MyThread : public Thread {
    MyThread(int numThreads)
    {
    }

    virtual void run()
    {
      mutex.lock();
      int myID = nextFreeID++;
#ifdef __LINUX__
      printf("ID no %i runs on core %i\n",myID,sched_getcpu());
#endif
      mutex.unlock();

      barrier.wait();
      double t0 = getSysTime();
      int lastPing = 0;
      while (1) {
        barrier.wait();
        if (done) {
          break;
        }
        mutex.lock();
        sum += 1;
        mutex.unlock();
        barrier.wait();

        double t1 = getSysTime();

        if (myID == 0) {
          int numSecs = int(t1-t0);
          if (numSecs > lastPing) {
            printf("after t=%5.fs: num ops =%8li, that's %f ops/sec\n",
                   t1-t0,sum,sum/(t1-t0));
            lastPing = numSecs;
          }
        }
        if (myID == 0 && (t1 - t0) >= timeToRun)
          done = true;
      }

      if (myID == 0)
        cout << "total number of OPs: " << sum << endl;
    }
    
  };

  int numThreads = 10;
  int firstThreadID = 1;

  extern "C" int main(int ac, char **av)
  {
#ifdef __LINUX__
    printf("main thread runs on core %i\n",sched_getcpu());
#endif
    printf("address of global variables is %li\n",(long)&sum);

    for (int i=1;i<ac;i++) {
      const std::string arg = av[i];
      if (arg == "-nt")
        numThreads = atoi(av[++i]);
      else if (arg == "-ft")
        firstThreadID = atoi(av[++i]);
      else if (arg == "-ns")
        timeToRun = atof(av[++i]);
      else
        throw std::runtime_error("unknown parameter '"+arg+"'");
    }
    
    barrier.init(numThreads);
    MyThread **thread = new MyThread *[numThreads];
    for (int i=0;i<numThreads;i++) {
      thread[i] = new MyThread(numThreads);
      thread[i]->start(firstThreadID+i);
    }

    while (1) {
      sleep(1);
      if (done) break;
    }


    for (int i=0;i<numThreads;i++)
      thread[i]->join();
    return 0;
  }

}
