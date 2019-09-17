// ======================================================================== //
// Copyright 2016-2019 Intel Corporation                                    //
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

#include <atomic>
#include <chrono>
#include <random>
#include <thread>
#include "maml/maml.h"
#include "ospcommon/tasking/tasking_system_handle.h"

struct MyHandler : public maml::MessageHandler
{
  MyHandler() = default;

  void incoming(const std::shared_ptr<maml::Message> &) override
  {
    ++numReceived;
  }

  std::atomic<int> numReceived;
};

extern "C" int main(int ac, char **av)
{
  MPI_CALL(Init(&ac, &av));
  ospcommon::tasking::initTaskingSystem();
  maml::init();

  std::mt19937 rng(std::random_device{}());
  std::uniform_int_distribution<int> distrib(0, 255);

  int numRuns  = 1000000;
  int rank     = -1;
  int numRanks = 0;
  MPI_CALL(Comm_size(MPI_COMM_WORLD, &numRanks));
  MPI_CALL(Comm_rank(MPI_COMM_WORLD, &rank));

  int numMessages = 100;
  int payloadSize = 100000;

  MyHandler handler;
  maml::registerHandlerFor(MPI_COMM_WORLD, &handler);

  char *payload = (char *)malloc(payloadSize);
  for (int i = 0; i < payloadSize; i++)
    payload[i] = distrib(rng);

  for (int run = 0; run < numRuns; run++) {
    MPI_CALL(Barrier(MPI_COMM_WORLD));
    double t0 = ospcommon::getSysTime();
    maml::start();

    for (int mID = 0; mID < numMessages; mID++) {
      for (int r = 0; r < numRanks; r++) {
        maml::sendTo(MPI_COMM_WORLD,
                     r,
                     std::make_shared<maml::Message>(payload, payloadSize));
      }
    }

    while (handler.numReceived != numRanks * numMessages * (run + 1)) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    maml::stop();
    double t1        = ospcommon::getSysTime();
    double bytes     = numRanks * numMessages * payloadSize / (t1 - t0);
    std::string rate = ospcommon::prettyNumber(bytes);
    printf("rank %i: received %i messages in %lf secs; that is %sB/s\n",
           rank,
           numRanks * numMessages,
           t1 - t0,
           rate.c_str());
    MPI_CALL(Barrier(MPI_COMM_WORLD));
  }

  maml::shutdown();

  MPI_CALL(Barrier(MPI_COMM_WORLD));
  MPI_Finalize();
}
