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
      // std::cout << "mpi unlocked by " << g_whoHasTheMPILock << std::endl;
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
      : comm(other.comm), rank(other.rank), size(other.size), containsMe(other.containsMe)
    {
    }

    void Group::barrier() const
    {
      lockMPI("mpi::Group::barrier");
      MPI_CALL(Barrier(comm)); 
      unlockMPI();
    }

    void checkMpiError(int rc)
    {
      if (rc != MPI_SUCCESS)
        throw std::runtime_error("MPI Error");
    }

    
    /*! set to given intercomm, and properly set size, root, etc */
    void Group::setTo(MPI_Comm comm)
    {
      this->comm = comm;
      if (comm == MPI_COMM_NULL) {
        rank = size = -1;
      } else {
        int isInter;
        lockMPI("mpi::Group::setTo");
        MPI_CALL(Comm_test_inter(comm,&isInter));
        unlockMPI();
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
      lockMPI("mpi::Group::dup");
      MPI_CALL(Comm_dup(comm,&duped));
      unlockMPI();
      return Group(duped);
    }
        
    void init(int *ac, const char **av)
    {
      int initialized = false;
      lockMPI("mpi::init");
      MPI_CALL(Initialized(&initialized));
      unlockMPI();
      
      if (!initialized) {
        // MPI_Init(ac,(char ***)&av);
        int required = MPI_THREAD_SERIALIZED;
        int provided = 0;
        lockMPI("MPI_init_thread");
        MPI_CALL(Init_thread(ac,(char ***)&av,required,&provided));
        unlockMPI();
        if (provided != required) {
          throw std::runtime_error("MPI implementation does not offer "
                                   "multi-threading capabilities");
        }
        std::cout << "#osp.mpi: MPI_Init successful!" << std::endl;
      }
      else
      {
        printf("running ospray in pre-initialized mpi mode\n");
        int provided;
        MPI_Query_thread(&provided);
        int requested = MPI_THREAD_SERIALIZED;
        if (provided != requested)
          throw std::runtime_error("ospray requires mpi to be initialized with "
            "MPI_THREAD_SERIAL if initialized before calling ospray");
      }
      lockMPI("mpi::init()");
      world.comm = MPI_COMM_WORLD;
      MPI_CALL(Comm_rank(MPI_COMM_WORLD,&world.rank));
      MPI_CALL(Comm_size(MPI_COMM_WORLD,&world.size));
      unlockMPI();

      {
        /*! iw, jan 2017: this entire code block should eventually get
            removed once we switch to the new asyn messaging library;
            in particular the asyn messaging should be turned on by
            whoever needs it (eg, DFB, display wall, etc); not in
            general as soon as one initializes mpicommon ... (_and_
            the messaging code isn't exactly up to snuff wrt to
            cleanliness etc as it should be*/
        if (logMPI)
          std::cout << "#osp.mpi: starting up the async messaging layer ...!" << std::endl;
        mpi::async::CommLayer::WORLD = new mpi::async::CommLayer;
        mpi::async::Group *worldGroup =
          mpi::async::createGroup(MPI_COMM_WORLD,
                                  mpi::async::CommLayer::WORLD,
                                  290374);
        assert(worldGroup);
        mpi::async::CommLayer::WORLD->group = worldGroup;
        if (logMPI)
          std::cout << "#osp.mpi: async messaging layer started ... " << std::endl;
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
      if (logMPI)
        std::cout << "#osp.mpi: FOR NOW, WE DO _NOT_ LOCK MPI CALLS UPON STARTUP - EVENTUALLY WE HAVE TO DO THIS!!!" << std::endl;
#else
      lockMPI();
#endif
    }

    // void send(const Address& addr, work::Work* work)
    // {
    //   BufferedMPIComm::get()->send(addr, work);
    // }

    // void recv(const Address& addr, std::vector<work::Work*>& work)
    // {
    //   BufferedMPIComm::get()->recv(addr, work);
    // }

    // void flush()
    // {
    //   BufferedMPIComm::get()->flush();
    // }

    // void barrier(const Group& group)
    // {
    //   BufferedMPIComm::get()->barrier(group);
    // }
  } // ::ospray::mpi
} // ::ospray
