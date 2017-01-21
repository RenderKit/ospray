// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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

#include "MPICommon.h"
#include "async/CommLayer.h"
#include "BufferedMPIComm.h"

namespace ospray {
  namespace mpi {

    OSPRAY_MPI_INTERFACE Group world;
    OSPRAY_MPI_INTERFACE Group app;
    OSPRAY_MPI_INTERFACE Group worker;

    /*! constructor. sets the 'comm', 'rank', and 'size' fields */
    Group::Group(MPI_Comm initComm)
    {
      setTo(initComm);
    }

      /*! set to given intercomm, and properly set size, root, etc */
    void Group::setTo(MPI_Comm comm)
    {
      this->comm = comm;
      if (comm == MPI_COMM_NULL) {
        rank = size = -1;
      } else {
        int isInter;
        MPI_CALL(Comm_test_inter(comm,&isInter));
        if (isInter)
          makeInterComm(comm);
        else
          makeIntraComm(comm);
      }
    }

    /*! do an MPI_Comm_dup, and return duplicated communicator */
    Group Group::dup() const
    {
      MPI_Comm duped;
      MPI_CALL(Comm_dup(comm,&duped));
      return Group(duped);
    }
        
    void init(int *ac, const char **av)
    {
      int initialized = false;
      MPI_CALL(Initialized(&initialized));

      if (!initialized) {
        // MPI_Init(ac,(char ***)&av);
        int required = MPI_THREAD_MULTIPLE;
        int provided = 0;
        MPI_CALL(Init_thread(ac,(char ***)&av,required,&provided));
        if (provided != required) {
          throw std::runtime_error("MPI implementation does not offer "
                                   "multi-threading capabilities");
        }
      }
      else
      {
        printf("running ospray in pre-initialized mpi mode\n");
        int provided;
        MPI_Query_thread(&provided);
        int requested = MPI_THREAD_MULTIPLE;
        if (provided != requested)
          throw std::runtime_error("ospray requires mpi to be initialized with "
            "MPI_THREAD_MULTIPLE if initialized before calling ospray");
      }
      world.comm = MPI_COMM_WORLD;
      MPI_CALL(Comm_rank(MPI_COMM_WORLD,&world.rank));
      MPI_CALL(Comm_size(MPI_COMM_WORLD,&world.size));

      mpi::async::CommLayer::WORLD = new mpi::async::CommLayer;
      mpi::async::Group *worldGroup =
          mpi::async::createGroup(MPI_COMM_WORLD,
                                  mpi::async::CommLayer::WORLD,
                                  290374);
      mpi::async::CommLayer::WORLD->group = worldGroup;
    }

    void send(const Address& addr, work::Work* work)
    {
      BufferedMPIComm::get()->send(addr, work);
    }

    void recv(const Address& addr, std::vector<work::Work*>& work)
    {
      BufferedMPIComm::get()->recv(addr, work);
    }

    void flush()
    {
      BufferedMPIComm::get()->flush();
    }

    void barrier(const Group& group)
    {
      BufferedMPIComm::get()->barrier(group);
    }
  } // ::ospray::mpi
} // ::ospray
