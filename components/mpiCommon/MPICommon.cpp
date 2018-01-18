// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

namespace mpicommon {

  OSPRAY_MPI_INTERFACE bool mpiIsThreaded = false;

  OSPRAY_MPI_INTERFACE Group world;
  OSPRAY_MPI_INTERFACE Group app;
  OSPRAY_MPI_INTERFACE Group worker;

  // Group definitions ////////////////////////////////////////////////////////

  /*! constructor. sets the 'comm', 'rank', and 'size' fields */
  Group::Group(MPI_Comm initComm)
  {
    setTo(initComm);
  }

  /*! constructor. sets the 'comm', 'rank', and 'size' fields */
  Group::Group(const Group &other)
    : containsMe(other.containsMe), comm(other.comm),
      rank(other.rank), size(other.size)
  {
  }

  void Group::makeIntraComm()
  {
    MPI_CALL(Comm_rank(comm,&rank));
    MPI_CALL(Comm_size(comm,&size));
    containsMe = true;
  }

  void Group::makeIntraComm(MPI_Comm _comm)
  {
    this->comm = _comm; makeIntraComm();
  }

  void Group::makeInterComm()
  {
    containsMe = false;
    rank = MPI_ROOT;
    MPI_CALL(Comm_remote_size(comm, &size));
  }

  void Group::makeInterComm(MPI_Comm _comm)
  {
    this->comm = _comm; makeInterComm();
  }

  void Group::barrier() const
  {
    MPI_CALL(Barrier(comm));
  }

  /*! set to given intercomm, and properly set size, root, etc */
  void Group::setTo(MPI_Comm _comm)
  {
    this->comm = _comm;
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

  // Message definitions //////////////////////////////////////////////////////

  /*! create a new message with given amount of bytes in storage */
  Message::Message(size_t size) : size(size)
  {
    data = (ospcommon::byte_t*)malloc(size);
  }

  /*! create a new message with given amount of storage, and copy
    memory from the given address to it */
  Message::Message(const void *copyMem, size_t size) : Message(size)
  {
    if (copyMem == nullptr)
      OSPRAY_THROW("#mpicommon: cannot create a message from a null pointer!");

    memcpy(data, copyMem, size);
  }

    /*! create a new message (addressed to given comm:rank) with given
        amount of storage, and copy memory from the given address to
        it */
  Message::Message(MPI_Comm comm, int rank,
                   const void *copyMem, size_t size)
    : Message(copyMem, size)
  {
    this->comm = comm;
    this->rank = rank;
  }

  /*! destruct message and free allocated memory */
  Message::~Message()
  {
    free(data);
  }

  bool Message::isValid() const
  {
    return comm != MPI_COMM_NULL && rank >= 0;
  }

  bool init(int *ac, const char **av, bool useCommWorld)
  {
    int initialized = false;
    MPI_CALL(Initialized(&initialized));

    int provided = 0;
    if (!initialized) {
      /* MPI not initialized by the app - it's up to us */
      MPI_CALL(Init_thread(ac, const_cast<char ***>(&av),
                           MPI_THREAD_MULTIPLE, &provided));
    } else {
      /* MPI was already initialized by the app that called us! */
      MPI_Query_thread(&provided);
    }
    mpiIsThreaded = provided == MPI_THREAD_MULTIPLE;

    if (useCommWorld) {
      MPI_CALL(Comm_dup(MPI_COMM_WORLD, &world.comm));
      MPI_CALL(Comm_rank(world.comm, &world.rank));
      MPI_CALL(Comm_size(world.comm, &world.size));
    }
    return !initialized;
  }

} // ::mpicommon
