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

#define OMPI_SKIP_MPICXX 1
#include <mpi.h>

// ospcommon
#include "ospcommon/common.h"
#include "ospray/common/OSPCommon.h"

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
#define MPI_CALL(a) { \
  int rc = MPI_##a; \
  if (rc != MPI_SUCCESS) \
    throw std::runtime_error("MPI call returned error"); \
}

#define SERIALIZED_MPI_CALL(a) { \
  ospray::mpi::serialized(CODE_LOCATION, [&]() { \
    MPI_CALL(a) \
  }); \
}

// Log level at which extremely verbose MPI logging output will
// be written
#define OSPRAY_MPI_VERBOSE_LEVEL 3

namespace ospray {
  namespace mpi {
    using namespace ospcommon;
    
    /*! global variable that turns on logging of MPI communication
      (for debugging) _may_ eventually turn this into a real logLevel,
      but for now tihs is cleaner here thatn in the MPI device
    */
    OSPRAY_MPI_INTERFACE extern bool logMPI;
    OSPRAY_MPI_INTERFACE extern bool mpiIsThreaded;

    /*! helper functions that lock resp unlock the mpi serializer
      mutex the 'whohasthelock' variable is only for debugging - it
      allows for querying who acutally has the lock; the value of that
      parameter is only for human consumptoin, though - we do not
      defined in any way what this value is supposed to be
     */
    OSPRAY_MPI_INTERFACE void lockMPI(const char *whoHasTheLock);

    /*! helper functions that lock resp unlock the mpi serializer mutex */
    OSPRAY_MPI_INTERFACE void unlockMPI();

    template<typename CLOSURE_T>
    inline void serialized(const char *lockId, CLOSURE_T&& criticalSection)
    {
      lockMPI(lockId);
      criticalSection();
      unlockMPI();
    }

    /*! the value of the 'whoHasTheLock' parameter of the last
        succeeding lockMPI() call */
    OSPRAY_MPI_INTERFACE const char *whoHasTheMPILock();
    
    /*! use this macro as a lock-guard inside any scope you want to
        perform MPI calls in */
#define SERIALIZE_MPI \
  std::lock_guard<std::mutex>(ospray::mpi::mpiSerializerMutex);
    
    //! abstraction for an MPI group. 
    /*! it's the responsiblity of the respective mpi setup routines to
      fill in the proper values */
    struct OSPRAY_MPI_INTERFACE Group
    {
      /*! constructor. sets the 'comm', 'rank', and 'size' fields */
      Group(MPI_Comm initComm = MPI_COMM_NULL);
      Group(const Group &other);

      inline bool valid() const
      {
        return comm != MPI_COMM_NULL; 
      }

      // this is the RIGHT naming convention - old code has them all inside out.
      void makeIntraComm();
      void makeIntraComm(MPI_Comm comm);
      void makeInterComm(MPI_Comm comm);
      void makeInterComm();

      /*! set to given intercomm, and properly set size, root, etc */
      void setTo(MPI_Comm comm);

      /*! do an MPI_Comm_dup, and return duplicated communicator */
      Group dup() const;

      /*! perform a MPI_barrier on this communicator */
      void barrier() const;
      
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

    //! abstraction for any other peer node that we might want to communicate with
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
    OSPRAY_MPI_INTERFACE void flush();

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

    inline bool IamTheMaster()
    {
      return mpi::world.rank == 0;
    }

    // RTTI hash ID lookup helper functions ///////////////////////////////////

    OSPRAY_MPI_INTERFACE size_t translatedHash(size_t v);

    template <typename T>
    inline size_t typeIdOf()
    {
      return translatedHash(typeid(T).hash_code());
    }

    template <typename T>
    inline size_t typeIdOf(T *v)
    {
      return translatedHash(typeid(*v).hash_code());
    }

    template <typename T>
    inline size_t typeIdOf(const T &v)
    {
      return translatedHash(typeid(v).hash_code());
    }

    template <typename T>
    inline size_t typeIdOf(const std::unique_ptr<T> &v)
    {
      return translatedHash(typeid(*v).hash_code());
    }

    template <typename T>
    inline size_t typeIdOf(const std::shared_ptr<T> &v)
    {
      return translatedHash(typeid(*v).hash_code());
    }

    // RTTI string name lookup helper functions ///////////////////////////////

    template <typename T>
    inline std::string typeString()
    {
      return typeid(T).name();
    }

    template <typename T>
    inline std::string typeString(T *v)
    {
      return typeid(*v).name();
    }

    template <typename T>
    inline std::string typeString(const T &v)
    {
      return typeid(v).name();
    }

    template <typename T>
    inline std::string typeString(const std::unique_ptr<T> &v)
    {
      return typeid(*v).name();
    }

    template <typename T>
    inline std::string typeString(const std::shared_ptr<T> &v)
    {
      return typeid(*v).name();
    }

  }// ::ospray::mpi
} // ::ospray
