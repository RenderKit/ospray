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

// Log level at which extremely verbose MPI logging output will
// be written
#define OSPRAY_MPI_VERBOSE_LEVEL 3

#define OSPRAY_WORLD_GROUP_TAG 290374

namespace ospray {
  namespace mpi {
    using namespace ospcommon;
    
    /*! global variable that turns on logging of MPI communication
      (for debugging) _may_ eventually turn this into a real logLevel,
      but for now tihs is cleaner here thatn in the MPI device
    */
    OSPRAY_MPI_INTERFACE extern bool logMPI;
    OSPRAY_MPI_INTERFACE extern bool mpiIsThreaded;

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

    inline int globalRank()
    {
      return mpi::world.rank;
    }

    inline int numGlobalRanks()
    {
      return mpi::world.size;
    }

    inline int numWorkers()
    {
      return mpi::world.size - 1;
    }

    inline int workerRank()
    {
      return mpi::worker.rank;
    }

    inline bool isMpiParallel()
    {
      return numWorkers() > 0;
    }

    inline int32 masterRank()
    {
      return 0;
    }

    inline bool IamTheMaster()
    {
      return mpi::world.rank == masterRank();
    }

    inline bool IamAWorker()
    {
      return mpi::world.rank > 0;
    }

    inline int workerRankFromGlobalRank(int globalRank)
    {
      return globalRank - 1;
    }

    inline int globalRankFromWorkerRank(int workerRank)
    {
      return 1 + workerRank;
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
