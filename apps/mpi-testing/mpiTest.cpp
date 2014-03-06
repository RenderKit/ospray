#include "mpi/mpicommon.h"
#include "ospray/ospray.h"
#include "common/ospcommon.h"


namespace ospray {
  using std::cout;
  using std::endl;

  void printNice(double val)
  {
    char *ooms = " KMGTP";
    int oom = 0;
    while (val >= 100.) {
      val /= 1024.f;
      ++oom;
    }
    // if (val >= 10.)
      printf("%4.2f%c",val,ooms[oom]);
    // else if (val >= 1.)
    //   printf("%2.1f%c",val,ooms[oom]);
    // else
    //   printf("%2.1f%c",val,ooms[oom]);
  }

  void printClientBW(int workerID,int msgSize)
  {
    MPI_Send(&msgSize,1,MPI_INT,workerID,0,mpi::worker.comm);
    if (msgSize == -1) return;
    double t0 = getSysTime();
    double t;
    char data[msgSize];
    bzero(data,msgSize);
    for (int i=0;i<msgSize;i++)
      data[i] = random();
    long numBytesSent = 0;
    long numPacketsSent = 0;
    MPI_Status status;

    long repeats = 100;
    while (1) {
      data[0] = 0;
      for (int i=0;i<repeats;i++) {
        MPI_Send(data,msgSize,MPI_CHAR,workerID,0,mpi::worker.comm);
        numBytesSent += msgSize;
        numPacketsSent++;
        MPI_Recv(data,msgSize,MPI_CHAR,workerID,0,mpi::worker.comm,&status);
      }
      t = getSysTime()-t0;
      if (t > 5.f) {
        data[0] = 1;
        MPI_Send(data,msgSize,MPI_CHAR,workerID,0,mpi::worker.comm);
        break;
      }
    }
    double bw = numBytesSent / t;
    // PRINT(t);
    // PRINT(numBytesSent);
    // PRINT(numPacketsSent);
    cout << "[";
    printNice(bw);
    cout << "/s]";
  }

  void mpiTest()
  {
    if (mpi::worker.size < 1) {
      throw std::runtime_error("no mpi workers !?");
    }
    cout << "found " << mpi::worker.size << " workers" << endl;

    cout << "--------------------------------------------" << endl;
    cout << "bandwidth master to each worker [64b][256b][1kb][4kb][16kb][64kb]" << endl;
    for (int i=0;i<mpi::worker.size;i++) {
      printf("worker %i:",i);
      printClientBW(i,64);
      printClientBW(i,256);
      printClientBW(i,1024);
      printClientBW(i,4*1024);
      printClientBW(i,16*1024);
      printClientBW(i,64*1024);
      printClientBW(i,-1);
      cout << endl;
    };
  }

}

int main(int ac, const char **av)
{
  ospInit(&ac,av);
  ospLoadModule("MPItest");
  
  PING;
  PING;
  PING;
  ospray::mpiTest();

  std::cout << "done mpitest" << std::endl;
}


