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

#include "maml/maml.h"
#include <atomic>
#include <sys/times.h>

namespace maml {  
} // ::maml

struct MyHandler : public maml::MessageHandler
{
  MyHandler() : numReceived() {};
  
  virtual void incoming(const std::shared_ptr<maml::Message> &message)
  {
    ++numReceived;
  }

  std::atomic<int> numReceived;
};
  
extern "C" int main(int ac, char **av)
{
  MPI_CALL(Init(&ac, &av));
  maml::init(ac,av);
  srand48(times(NULL));

  int numRuns = 1000000;
  int rank = -1;
  int numRanks = 0;
  MPI_CALL(Comm_size(MPI_COMM_WORLD,&numRanks));
  MPI_CALL(Comm_rank(MPI_COMM_WORLD,&rank));

  int numMessages = 100;
  int payloadSize = 100000;
  
  MyHandler handler;
  maml::registerHandlerFor(MPI_COMM_WORLD,&handler);

  char *payload = (char*)malloc(payloadSize);
  for (int i=0;i<payloadSize;i++)
    payload[i] = drand48()*256;
  
  for (int run=0;run<numRuns;run++) {
    MPI_CALL(Barrier(MPI_COMM_WORLD));
    double t0 = ospcommon::getSysTime();
    maml::start();
    
    for (int mID=0;mID<numMessages;mID++) {
      for (int r=0;r<numRanks;r++) {
        // usleep(1000*drand48());
        maml::sendTo(MPI_COMM_WORLD,r,std::make_shared<maml::Message>(payload,payloadSize));
        // maml::sendTo(MPI_COMM_WORLD,r,new maml::Message(&r,sizeof(r)));
      }
    }

    while (handler.numReceived != numRanks*numMessages*(run+1)) {
      // printf("rank %i received %i/%i (maml::state %i)\n",
      //        rank,(int)handler.numReceived,numRanks*numMessages*(run+1),
      //        maml::state);
      usleep(10000);
    }

    maml::flush();
    maml::stop();
    double t1 = ospcommon::getSysTime();
    double bytes = numRanks * numMessages * payloadSize / (t1-t0);
    std::string rate = ospcommon::prettyNumber(bytes);
    printf("rank %i: received %i messages in %lf secs; that is %sB/s\n",rank,numRanks*numMessages,t1-t0,
           rate.c_str());
    MPI_CALL(Barrier(MPI_COMM_WORLD));
  }
  
  MPI_CALL(Barrier(MPI_COMM_WORLD));
  MPI_Finalize();
}
