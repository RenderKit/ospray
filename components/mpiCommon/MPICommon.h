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

#pragma once

// std
#include <memory>
#include <vector>
// mpi
#include <mpi.h>
// ospcommon
#include "ospcommon/common.h"
#include "OSPMPIConfig.h"

#ifdef _WIN32
#  ifdef ospray_mpi_common_EXPORTS
#    define OSPRAY_MPI_INTERFACE __declspec(dllexport)
#  else
#    define OSPRAY_MPI_INTERFACE __declspec(dllimport)
#  endif
#else
#  define OSPRAY_MPI_INTERFACE
#endif

// IMPI on Windows defines MPI_CALL already, erroneously
#ifdef MPI_CALL
# undef MPI_CALL
#endif
/*! helper macro that checks the return value of all MPI_xxx(...)
    calls via MPI_CALL(xxx(...)).  */
#define MPI_CALL(a) { int rc = MPI_##a; if (rc != MPI_SUCCESS) throw std::runtime_error("MPI call returned error"); }

namespace ospray {
  namespace mpi {

    inline void checkMpiError(int rc)
    {
      if (rc != MPI_SUCCESS)
        throw std::runtime_error("MPI Error");
    }

    //! abstraction for an MPI group. 
    /*! it's the responsiblity of the respective mpi setup routines to
      fill in the proper values */
    struct Group
    {
      /*! constructor. sets the 'comm', 'rank', and 'size' fields */
      Group(MPI_Comm initComm=MPI_COMM_NULL);
      
      // this is the RIGHT naming convention - old code has them all inside out.
      void makeIntraComm() 
      { MPI_Comm_rank(comm,&rank); MPI_Comm_size(comm,&size); containsMe = true; }
      void makeIntraComm(MPI_Comm comm)
      { this->comm = comm; makeIntraComm(); }
      void makeInterComm(MPI_Comm comm)
      { this->comm = comm; makeInterComm(); }
      void makeInterComm()
      { containsMe = false; rank = MPI_ROOT; MPI_Comm_remote_size(comm,&size); }

      /*! set to given intercomm, and properly set size, root, etc */
      void setTo(MPI_Comm comm);

      /*! do an MPI_Comm_dup, and return duplicated communicator */
      Group dup() const;

      /*! perform a MPI_barrier on this communicator */
      void barrier() const { MPI_CALL(Barrier(comm)); }
      
      /*! whether the current process/thread is a member of this
        gorup */
      bool containsMe {false};
      /*! communictor for this group. intercommunicator if i'm a
        member of this gorup; else it's an intracommunicator */
      MPI_Comm comm {MPI_COMM_NULL};
      /*! my rank in this group if i'm a member; else set to
        MPI_ROOT */
      int rank {-1};
      /*! size of this group if i'm a member, else size of remote
        group this intracommunicaotr refers to */
      int size {-1};
    };

    // //! abstraction for any other peer node that we might want to communicate with
    struct Address
    {
      //! group that this peer is in
      Group *group;
      //! this peer's rank in this group
      int rank;
        
      Address(Group *group = nullptr, int rank = -1)
        : group(group), rank(rank) {}
      inline bool isValid() const { return group != nullptr && rank >= 0; }
    };

    inline bool operator==(const Address &a, const Address &b)
    {
      return a.group == b.group && a.rank == b.rank;
    }

    inline bool operator!=(const Address &a, const Address &b)
    {
      return !(a == b);
    }

    //special flags for sending and reciving from all ranks instead of individuals
    const int SEND_ALL = -1;
    const int RECV_ALL = -1;

    //! MPI_COMM_WORLD
    OSPRAY_MPI_INTERFACE extern Group world;
    /*! for workers: intracommunicator to app
      for app: intercommunicator among app processes
      */
    OSPRAY_MPI_INTERFACE extern Group app;
    /*!< group of all ospray workers (often the
      world root is reserved for either app or
      load balancing, and not part of the worker
      group */
    OSPRAY_MPI_INTERFACE extern Group worker;

    // Initialize OSPRay's MPI groups
    OSPRAY_MPI_INTERFACE void init(int *ac, const char **av);

    namespace work {
      struct Work;
    }

    // OSPRAY_INTERFACE void send(const Address& address, void* msgPtr, int32 msgSize);
    OSPRAY_MPI_INTERFACE void send(const Address& addr, work::Work* work);
    //TODO: callback?
    OSPRAY_MPI_INTERFACE void recv(const Address& addr, std::vector<work::Work*>& work);
    // OSPRAY_INTERFACE void send(const Address& addr, )
    OSPRAY_MPI_INTERFACE void flush();
    OSPRAY_MPI_INTERFACE void barrier(const Group& group);

    inline int getWorkerCount()
    {
      return mpi::worker.size;
    }

    inline int getWorkerRank()
    {
      return mpi::worker.rank;
    }

    inline bool isMpiParallel()
    {
      return getWorkerCount() > 0;
    }

  }// ::ospray::mpi
} // ::ospray
