// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "ospray/mpi/async/Messaging.h"

#define ASYNC_TAG 123

#define NUM_START_MESSAGES_PER_NODE 3
// min num ints we're sending
#define MIN_SIZE 64
// max num ints we're sending
#define MAX_SIZE (4096)
// #define MAX_SIZE (4096*1024)
// num seconds we're sending back and forth
#define NUM_SECONDS 3

#define FIXED_SIZE 499

namespace ospray {

int numTerminated = 0;
Condition cond;
Mutex mutex;
bool doNotSendAnyMoreMessages = false;
size_t numBytesReceived = 0;
size_t numMessagesReceived = 0;
// bool      checkSum = false;

int computeCheckSum1toN(int *arr, int N)
{
  int sum = 0;
  for (int i = 1; i < N; i++)
    sum += arr[i];
  return sum;
}
void sendRandomMessage(mpi::Address addr)
{
#ifdef FIXED_SIZE
  int N = FIXED_SIZE;
#else
  int N = MIN_SIZE + int(drand48() * (MAX_SIZE - MIN_SIZE));
#endif
  int *msg = (int *)malloc(N * sizeof(N)); // new int[N];
  for (int i = 1; i < N; i++)
    msg[i] = rand();
  msg[0] = computeCheckSum1toN(msg, N);
  mpi::async::send(addr, msg, N * sizeof(int));
}

struct CheckSumAndBounceNewRandomMessage : public mpi::async::Consumer
{
  virtual void process(const mpi::Address &source, void *message, int32 size)
  {
#ifdef FIXED_SIZE
    if (size != FIXED_SIZE * sizeof(int)) {
      printf("rank %i received %i\n", mpi::world.rank, size);
      fflush(0);
    }
#endif

    // printf("rank %i received %i\n",mpi::world.rank,size); fflush(0);

    if (size == 4) {
      mutex.lock();
      numTerminated++;
      printf("rank %i terminated %i\n", mpi::world.rank, numTerminated);
      fflush(0);
      cond.broadcast();
      mutex.unlock();
      free(message);
      return;
    }

    // if (checkSum) {
    //   // -------------------------------------------------------
    //   // checksum
    //   // -------------------------------------------------------
    //   int *msg = (int *)message;
    //   int N = size/sizeof(int);
    //   int checkSum = computeCheckSum1toN(msg,N);
    //   if (msg[0] != checkSum)
    //     throw std::runtime_error("invalid checksum!");
    //   free(message);

    //   mutex.lock();
    //   numMessagesReceived ++;
    //   numBytesReceived += size;
    //   if (!doNotSendAnyMoreMessages) {
    //     int tgt = rand() % source.group->size;
    //     sendRandomMessage(mpi::Address(source.group,tgt));
    //   }
    //   mutex.unlock();
    // } else {
    mutex.lock();
    numMessagesReceived++;
    numBytesReceived += size;
    if (!doNotSendAnyMoreMessages) {
      int tgt = rand() % source.group->size;
      mpi::async::send(mpi::Address(source.group, tgt), message, size);
    }
    mutex.unlock();
    // }
  }
};

void mpiCommTest(int &ac, char **av)
{
  mpi::init(&ac, (const char **)av);

  CheckSumAndBounceNewRandomMessage consumer;

  mpi::async::Group *world =
      mpi::async::createGroup("world", MPI_COMM_WORLD, &consumer, ASYNC_TAG);
  srand(world->rank * 13 * 17 * 23 + 2342556);

  mpi::world.barrier();
  char hostname[1000];
  gethostname(hostname, 1000);
  printf("#osp:mpi(%2i/%i): async comm test on %s\n",
      mpi::world.rank,
      mpi::world.size,
      hostname);
  fflush(0);
  mpi::world.barrier();
  double t0 = getSysTime();

  for (int i = 0; i < mpi::world.size * NUM_START_MESSAGES_PER_NODE; i++) {
    // int tgt = (world->rank+i) % world->size;
    int tgt = rand() % world->size;
    sendRandomMessage(mpi::Address(world, tgt));
  }

  sleep(NUM_SECONDS);
  // usleep(NUM_SECONDS*1000000LL);

  mutex.lock();
  doNotSendAnyMoreMessages = true;
  for (int i = 0; i < world->size; i++) {
    int *msg = (int *)malloc(sizeof(int));
    *msg = -1;
    mpi::async::send(mpi::Address(world, i), msg, sizeof(int));
  }
  printf("#osp:mpi(%2i/%i): done notices sent\n",
      mpi::world.rank,
      mpi::world.size);
  fflush(0);
  while (numTerminated < mpi::world.size)
    cond.wait(mutex);
  mutex.unlock();
  double t1 = getSysTime();
  double MBs = numBytesReceived / (1024 * 1024.f);
  double secs = t1 - t0;
  printf(
      "#osp:mpi(%2i/%i): received %li msgs of %5.3lfMB in %2.1lfsecs, that's "
      "%5.3lfMB/sec\n",
      mpi::world.rank,
      mpi::world.size,
      numMessagesReceived,
      MBs,
      secs,
      MBs / secs);
  fflush(0);

  mpi::world.barrier();
  usleep(1000);
  mpi::async::shutdown();
}

} // namespace ospray

int main(int ac, char **av)
{
  ospray::mpiCommTest(ac, av);
  return 0;
}
