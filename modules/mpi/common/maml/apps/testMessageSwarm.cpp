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

/*! \file testMessageSwarm This implements a test scenario where each
    ray generates a set of messages (of given number and size) and
    sends them to random other ranks; every time a message is received
    it's simply bounced to another random node */

#include <atomic>
#include <chrono>
#include <random>
#include <thread>
#include "maml/maml.h"
#include "ospcommon/tasking/tasking_system_handle.h"

static int numRanks = 0;
static std::atomic<size_t> numReceived;

struct BounceHandler : public maml::MessageHandler
{
  std::mt19937 rng;
  std::uniform_int_distribution<> rank_distrib;

  BounceHandler() : rng(std::random_device{}()), rank_distrib(0, numRanks - 1)
  {
  }

  void incoming(const std::shared_ptr<maml::Message> &message) override
  {
    ++numReceived;
    int nextRank = rank_distrib(rng);
    maml::sendTo(MPI_COMM_WORLD, nextRank, message);
  }
};

extern "C" int main(int ac, char **av)
{
  MPI_CALL(Init(&ac, &av));
  ospcommon::tasking::initTaskingSystem();

  maml::init();

  int rank = -1;
  MPI_CALL(Comm_size(MPI_COMM_WORLD, &numRanks));
  MPI_CALL(Comm_rank(MPI_COMM_WORLD, &rank));

  std::mt19937 rng(std::random_device{}());
  std::uniform_int_distribution<int> distrib(0, 255);
  std::uniform_int_distribution<> rank_distrib(0, numRanks - 1);

  int numMessages = 100;
  int payloadSize = 100000;

  BounceHandler handler;
  maml::registerHandlerFor(MPI_COMM_WORLD, &handler);

  char *payload = (char *)malloc(payloadSize);
  for (int i = 0; i < payloadSize; i++)
    payload[i] = distrib(rng);

  maml::start();

  double t0 = ospcommon::getSysTime();
  for (int mID = 0; mID < numMessages; mID++) {
    int r = rank_distrib(rng);
    maml::sendTo(MPI_COMM_WORLD,
                 r,
                 std::make_shared<maml::Message>(payload, payloadSize));
  }

  while (1) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    double t1 = ospcommon::getSysTime();
    std::string numBytes =
        ospcommon::prettyNumber((size_t)numReceived * payloadSize);
    double rate            = (size_t)numReceived * payloadSize / (t1 - t0);
    std::string rateString = ospcommon::prettyNumber(rate);

    printf(
        "rank %i: received %li messages (%sbytes) in %lf secs; that is %sB/s\n",
        rank,
        (size_t)numReceived,
        numBytes.c_str(),
        t1 - t0,
        rateString.c_str());
  }

  /* this will never terminate ... */
}
