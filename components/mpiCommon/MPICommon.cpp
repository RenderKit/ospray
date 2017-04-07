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

namespace ospray {
  namespace mpi {

    /*! global variable that turns on logging of MPI communication
      (for debugging) _may_ eventually turn this into a real logLevel,
      but for now tihs is cleaner here thatn in the MPI device
     */
    OSPRAY_MPI_INTERFACE bool logMPI = 0;
    OSPRAY_MPI_INTERFACE bool mpiIsThreaded = 0;

    OSPRAY_MPI_INTERFACE Group world;
    OSPRAY_MPI_INTERFACE Group app;
    OSPRAY_MPI_INTERFACE Group worker;

    /*! for mpi versions that do not support MPI_THREAD_MULTIPLE
        (default openmpi, for example) it is not allows to perform
        concurrnt MPI_... calls from differnt threads. To avoid this,
        _all_ threads inside ospray should lock this global mutex
        before doing any MPI calls. Furthermore, this mutex will get
        unlocked _only_ while ospray is executing api call (we'll
        always lock it before we return to the calling app), thus we
        can make sure that no ospray mpi calls will ever interfere
        with the app */
    std::mutex mpiSerializerMutex;

    /*! the value of the 'whoHasTheLock' parameter of the last
        succeeding lockMPI() call */
    const char *g_whoHasTheMPILock = "<nobody - never been locked>";
    
    /*! the value of the 'whoHasTheLock' parameter of the last
        succeeding lockMPI() call */
    const char *whoHasTheMPILock()
    {
      return g_whoHasTheMPILock;
    }
    
    /*! helper functions that lock resp unlock the mpi serializer mutex */
    void lockMPI(const char *whoWantsTheLock)
    {
      mpiSerializerMutex.lock();
      g_whoHasTheMPILock = whoWantsTheLock;
    }

    /*! helper functions that lock resp unlock the mpi serializer mutex */
    void unlockMPI()
    {
      g_whoHasTheMPILock = "<nobody>";
      mpiSerializerMutex.unlock();
    }
    
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
      mpi::serialized(CODE_LOCATION, [&]() {
        MPI_CALL(Comm_rank(comm,&rank));
        MPI_CALL(Comm_size(comm,&size));
      });
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
      mpi::serialized(CODE_LOCATION, [&]() {
        containsMe = false;
        rank = MPI_ROOT;
        MPI_CALL(Comm_remote_size(comm, &size));
      });
    }

    void Group::barrier() const
    {
      SERIALIZED_MPI_CALL(Barrier(comm));
    }

    /*! set to given intercomm, and properly set size, root, etc */
    void Group::setTo(MPI_Comm comm)
    {
      this->comm = comm;
      if (comm == MPI_COMM_NULL) {
        rank = size = -1;
      } else {
        int isInter;
        SERIALIZED_MPI_CALL(Comm_test_inter(comm,&isInter));
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
      SERIALIZED_MPI_CALL(Comm_dup(comm,&duped));
      return Group(duped);
    }
        
    void init(int *ac, const char **av)
    {
      int initialized = false;
      SERIALIZED_MPI_CALL(Initialized(&initialized));
      
      int provided = 0;
      if (!initialized) {
        /* MPI not initialized by the app - it's up to us */
        int required = MPI_THREAD_MULTIPLE;
        SERIALIZED_MPI_CALL(Init_thread(ac,(char ***)&av,MPI_THREAD_MULTIPLE,&provided));
      } else {
        /* MPI was already initialized by the app that called us! */
        MPI_Query_thread(&provided);
      }

      int rank;
      SERIALIZED_MPI_CALL(Comm_rank(MPI_COMM_WORLD,&rank));
      switch(provided) {
      case MPI_THREAD_MULTIPLE:
        mpiIsThreaded = true;
        break;
      case MPI_THREAD_SERIALIZED:
        mpiIsThreaded = false;
        if (rank == 0) {
          postErrorMsg("#osp.mpi: didn't find 'thread_multiple' MPI, but!\n"
              "#osp.mpi: can still do thread_serialized.\n"
              "#osp.mpi: most things should work, but some modules.\n"
              "#osp.mpi: might not.\n", OSPRAY_MPI_VERBOSE_LEVEL);
        }
        break;
      default:
        throw std::runtime_error("fatal MPI error: MPI runtime doesn't offer even MPI_THREAD_SERIALIZED ...");
      }
      
      mpi::serialized(CODE_LOCATION, [&]() {
          world.comm = MPI_COMM_WORLD;
          MPI_CALL(Comm_rank(MPI_COMM_WORLD,&world.rank));
          MPI_CALL(Comm_size(MPI_COMM_WORLD,&world.size));
        });
      
      {
        /*! iw, jan 2017: this entire code block should eventually get
            removed once we switch to the new asyn messaging library;
            in particular the asyn messaging should be turned on by
            whoever needs it (eg, DFB, display wall, etc); not in
            general as soon as one initializes mpicommon ... (_and_
            the messaging code isn't exactly up to snuff wrt to
            cleanliness etc as it should be*/
        postErrorMsg("#osp.mpi: starting up the async messaging layer ...!", OSPRAY_MPI_VERBOSE_LEVEL);
        mpi::async::CommLayer::WORLD = new mpi::async::CommLayer;
        mpi::async::Group *worldGroup =
          mpi::async::createGroup(MPI_COMM_WORLD,
                                  mpi::async::CommLayer::WORLD,
                                  OSPRAY_WORLD_GROUP_TAG);
        assert(worldGroup);
        mpi::async::CommLayer::WORLD->group = worldGroup;
        postErrorMsg("#osp.mpi: async messaging layer started ... ", OSPRAY_MPI_VERBOSE_LEVEL);
      }

      // by default, all MPI comm gets locked down, unless we
      // explicitly enable it

#if 1
      /* "eventually", if we want to play nicely with apps we have to
         make sure that mpi calls are locked by default, get
         (temporarily) unlocked only during ospray API calls (when the
         app can expect that we do something), and re-locking it when
         the ospray call is done - this is the only way we can
         guarantee that nobody in ospray (say, an async messaging
         thread) is doing any mpi calls at the same time as the
         applicatoin (which would be bad). for now, i'm NOT yet doing
         this to find some other bugs, but eventually we have to do
         this - and once we do, we have to do the lock/unlock for
         every api call. */
      // Will: I think we're doing this now but in the device right?
      postErrorMsg("#osp.mpi: FOR NOW, WE DO _NOT_ LOCK MPI"
          " CALLS UPON STARTUP - EVENTUALLY WE HAVE TO DO THIS!!!",
          OSPRAY_MPI_VERBOSE_LEVEL);
#else
      lockMPI();
#endif
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
