// ======================================================================== //
// Copyright 2009-2014 Intel Corporation                                    //
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
#include "ospray/mpi/async/SimpleSendRecvMessaging.h"
#include "ospray/mpi/async/MultiIsendIrecvMessaging.h"

namespace ospray {
  namespace mpi {

    // Group *WORLD = NULL;

    // Group::Group(const std::string &name, MPI_Comm comm)
    //   : comm(comm), rank(-1), size(-1), name(name)
    // {
    //   int rc=MPI_SUCCESS;
    //   MPI_CALL(Comm_rank(comm,&rank));
    //   MPI_CALL(Comm_size(comm,&size));
    // }

    // void Group::barrier() 
    // {
    //   int rc=MPI_SUCCESS;
    //   MPI_CALL(Barrier(comm));
    // }

    // int requested = MPI_THREAD_MULTIPLE;
    // int provided = -1;

#if 0
    void initMPI(int &ac, char **&av)
    {
      // PING;fflush(0);
      if (WORLD != NULL)
        throw std::runtime_error("#osp:mpi: MPI already initialized.");

      // PING;fflush(0);
      MPI_Init_thread(&ac,&av,requested,&provided);
      // PING;fflush(0);
      if (provided != requested)
        throw std::runtime_error("#osp:mpi: the MPI implementation you are trying to run this application on does not support threading.");
      // world = new Group(MPI_COMM_WORLD);
      WORLD = new Group("MPI_COMM_WORLD");
      // PRINT(WORLD->toString());

      MPI_CALL(Barrier(MPI_COMM_WORLD));
      // printf("#osp:mpi: MPI Initialized, we are world rank %i/%i\n",WORLD->rank,WORLD->size);
      // fflush(0);
      MPI_CALL(Barrier(MPI_COMM_WORLD));
      printf("#osp:mpi: MPI Initialized, we are world rank %i/%i\n",
             WORLD->rank,WORLD->size);
      MPI_CALL(Barrier(WORLD->comm));
      PING;
    }
#endif

    namespace async {
      
      AsyncMessagingImpl *AsyncMessagingImpl::global = NULL;

      Group::Group(//const std::string &name, 
                   MPI_Comm comm, 
                   Consumer *consumer, int32 tag)
        :  tag(tag), consumer(consumer)
      {
        this->comm = comm;
        int rc=MPI_SUCCESS;
        MPI_CALL(Comm_rank(comm,&rank));
        MPI_CALL(Comm_size(comm,&size));
      }


      // init async layer
      void initAsync()
      {
        if (AsyncMessagingImpl::global == NULL) {
#if 0
          AsyncMessagingImpl::global = new MultiIsendIrecvImpl;
#else
          AsyncMessagingImpl::global = new SimpleSendRecvImpl;
#endif
          AsyncMessagingImpl::global->init();
        }
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
