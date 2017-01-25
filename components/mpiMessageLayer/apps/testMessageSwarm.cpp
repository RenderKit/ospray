// ************************************************************************** //
// Copyright 2016 Ingo Wald                                                   //
//                                                                            //
// Licensed under the Apache License, Version 2.0 (the "License");            //
// you may not use this file except in compliance with the License.           //
// You may obtain a copy of the License at                                    //
//                                                                            //
// http://www.apache.org/licenses/LICENSE-2.0                                 //
//                                                                            //
// Unless required by applicable law or agreed to in writing, software        //
// distributed under the License is distributed on an "AS IS" BASIS,          //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   //
// See the License for the specific language governing permissions and        //
// limitations under the License.                                             //
// ************************************************************************** //

/*! \file testMessageSwarm This implements a test scenario where each
    ray generates a set of messages (of given number and size) and
    sends them to random other ranks; every time a message is received
    it's simply bounced to anohter random node */

#include "maml/maml.h"
#include <atomic>
#include <sys/times.h>

int numRanks = 0;
std::atomic<size_t> numReceived;

struct BounceHandler : public maml::MessageHandler
{
  virtual void incoming(const std::shared_ptr<maml::Message> &message)
  {
    ++numReceived;
    int nextRank = (int)(drand48()*numRanks);
    maml::sendTo(MPI_COMM_WORLD,nextRank,message);
  }
};
  
extern "C" int main(int ac, char **av)
{
  MPI_CALL(Init(&ac, &av));
  maml::init(ac,av);
  srand48(times(NULL));

  int numRuns = 1000000;
  int rank = -1;
  MPI_CALL(Comm_size(MPI_COMM_WORLD,&numRanks));
  MPI_CALL(Comm_rank(MPI_COMM_WORLD,&rank));

  int numMessages = 100;
  int payloadSize = 100000;
  
  BounceHandler handler;
  maml::registerHandlerFor(MPI_COMM_WORLD,&handler);

  char *payload = (char*)malloc(payloadSize);
  for (int i=0;i<payloadSize;i++)
    payload[i] = drand48()*256;

  maml::start();
  
  double t0 = ospcommon::getSysTime();
  for (int mID=0;mID<numMessages;mID++) {
    int r = int(drand48()*numRanks);
    maml::sendTo(MPI_COMM_WORLD,r,std::make_shared<maml::Message>(payload,payloadSize));
  }
  
  while (1) {
    sleep(1);
    double t1 = ospcommon::getSysTime();
    std::string numBytes = ospcommon::prettyNumber((size_t)numReceived*payloadSize);
    double rate = (size_t)numReceived * payloadSize / (t1-t0);
    std::string rateString = ospcommon::prettyNumber(rate);
    printf("rank %i: received %li messages (%sbytes) in %lf secs; that is %sB/s\n",
           rank,(size_t)numReceived,numBytes.c_str(),t1-t0,rateString.c_str());
  }

  /* this will never terminate ... */
}
