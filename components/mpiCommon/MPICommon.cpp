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

#include <map>

namespace ospray {
  namespace mpi {

    /*! global variable that turns on logging of MPI communication
      (for debugging) _may_ eventually turn this into a real logLevel,
      but for now tihs is cleaner here thatn in the MPI device
     */
    OSPRAY_MPI_INTERFACE bool mpiIsThreaded = 0;

    OSPRAY_MPI_INTERFACE Group world;
    OSPRAY_MPI_INTERFACE Group app;
    OSPRAY_MPI_INTERFACE Group worker;
    
    /*! constructor. sets the 'comm', 'rank', and 'size' fields */
    Group::Group(MPI_Comm initComm)
    {
      setTo(initComm);
    }

    /*! constructor. sets the 'comm', 'rank', and 'size' fields */
    Group::Group(const Group &other)
      : comm(other.comm), rank(other.rank),
        size(other.size), containsMe(other.containsMe)
    {
    }

    void Group::makeIntraComm()
    {
      MPI_CALL(Comm_rank(comm,&rank));
      MPI_CALL(Comm_size(comm,&size));
      containsMe = true;
    }

    void Group::makeIntraComm(MPI_Comm comm)
    {
      this->comm = comm; makeIntraComm();
    }

    void Group::makeInterComm(MPI_Comm comm)
    {
      this->comm = comm; makeInterComm();
    }

    void Group::makeInterComm()
    {
      containsMe = false;
      rank = MPI_ROOT;
      MPI_CALL(Comm_remote_size(comm, &size));
    }

    void Group::barrier() const
    {
      MPI_CALL(Barrier(comm));
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
      
      int provided = 0;
      if (!initialized) {
        /* MPI not initialized by the app - it's up to us */
        MPI_CALL(Init_thread(ac, (char ***)&av,
                             MPI_THREAD_MULTIPLE, &provided));
      } else {
        /* MPI was already initialized by the app that called us! */
        MPI_Query_thread(&provided);
      }

      int rank;
      MPI_CALL(Comm_rank(MPI_COMM_WORLD,&rank));
      switch(provided) {
      case MPI_THREAD_MULTIPLE:
        mpiIsThreaded = true;
        break;
      case MPI_THREAD_SERIALIZED:
        mpiIsThreaded = false;
        if (rank == 0) {
          postStatusMsg("#osp.mpi: didn't find 'thread_multiple' MPI, but!\n"
              "#osp.mpi: can still do thread_serialized.\n"
              "#osp.mpi: most things should work, but some modules.\n"
              "#osp.mpi: might not.\n", OSPRAY_MPI_VERBOSE_LEVEL);
        }
        break;
      default:
        throw std::runtime_error("fatal MPI error: MPI runtime doesn't offer "
                                 "even MPI_THREAD_SERIALIZED ...");
      }
      
      world.comm = MPI_COMM_WORLD;
      MPI_CALL(Comm_rank(MPI_COMM_WORLD,&world.rank));
      MPI_CALL(Comm_size(MPI_COMM_WORLD,&world.size));
    }

    size_t translatedHash(size_t v)
    {
      static std::map<size_t, size_t> id_translation;

      auto itr = id_translation.find(v);
      if (itr == id_translation.end()) {
        static size_t newIndex = 0;
        id_translation[v] = newIndex;
        return newIndex++;
      } else {
        return id_translation[v];
      }
    }

  } // ::ospray::mpi
} // ::ospray
