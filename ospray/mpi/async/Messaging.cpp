// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

#include "mpi/async/Messaging.h"
#include "mpi/async/SimpleSendRecvMessaging.h"
#include "mpi/async/MultiIsendIrecvMessaging.h"
#include "mpi/async/BatchedIsendIrecvMessaging.h"

namespace ospray {
  namespace mpi {

    namespace async {
      
      AsyncMessagingImpl *AsyncMessagingImpl::global = NULL;

      Group::Group(//const std::string &name, 
                   MPI_Comm comm, 
                   Consumer *consumer, int32 tag)
        :  tag(tag), consumer(consumer)
      {
        int rc = MPI_SUCCESS;
        MPI_CALL(Comm_dup(comm,&this->comm));
        // this->comm = comm;
        MPI_CALL(Comm_rank(comm,&rank));
        MPI_CALL(Comm_size(comm,&size));
      }

      Group *WORLD = NULL;

      // init async layer
      void initAsync()
      {
        if (AsyncMessagingImpl::global == NULL) {
#if 1
          AsyncMessagingImpl::global = new BatchedIsendIrecvImpl;
#elif 1
          AsyncMessagingImpl::global = new MultiIsendIrecvImpl;
#else
          AsyncMessagingImpl::global = new SimpleSendRecvImpl;
#endif
          AsyncMessagingImpl::global->init();
        }

        // extern Group *WORLD;
      }

      Group *createGroup(const std::string &name, MPI_Comm comm, 
                         Consumer *consumer, int32 tag)
      {
        initAsync();
        return AsyncMessagingImpl::global->createGroup(name,comm,consumer,tag);
      }

      void shutdown()
      {
        AsyncMessagingImpl::global->shutdown();
      }

      void send(const Address &dest, void *msgPtr, int32 msgSize)
      {
        AsyncMessagingImpl::global->send(dest,msgPtr,msgSize);
      }

    } // ::ospray::mpi::async
  } // ::ospray::mpi
} // ::ospray
