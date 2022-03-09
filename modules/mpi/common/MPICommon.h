// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <chrono>
#include <memory>
#include <mutex>
#include <vector>
#define OMPI_SKIP_MPICXX 1
#include <mpi.h>

#include "ospray/OSPEnums.h"
#include "rkcommon/common.h"

// IMPI on Windows defines MPI_CALL already, erroneously
#ifdef MPI_CALL
#undef MPI_CALL
#endif
/*! helper macro that checks the return value of all MPI_xxx(...)
    calls via MPI_CALL(xxx(...)).  */
#define MPI_CALL(a)                                                            \
  {                                                                            \
    int rc = MPI_##a;                                                          \
    if (rc != MPI_SUCCESS)                                                     \
      throw std::runtime_error("MPI call returned error");                     \
  }

#define OSPRAY_THROW(a)                                                        \
  throw std::runtime_error(                                                    \
      "in " + std::string(__PRETTY_FUNCTION__) + " : " + std::string(a))

#define OSPRAY_WORLD_GROUP_TAG 290374

namespace mpicommon {
using namespace rkcommon;

/*! global variable that turns on logging of MPI communication
  (for debugging) _may_ eventually turn this into a real logLevel,
  but for now this is cleaner here than in the MPI device
*/
extern bool mpiIsThreaded;

//! abstraction for an MPI group.
/*! it's the responsibility of the respective mpi setup routines to
  fill in the proper values */
struct Group
{
  /*! constructor. sets the 'comm', 'rank', and 'size' fields */
  Group(MPI_Comm initComm = MPI_COMM_NULL);
  Group(const Group &other);

  inline bool valid() const
  {
    return comm != MPI_COMM_NULL;
  }

  void makeIntraComm();
  void makeIntraComm(MPI_Comm);

  void makeInterComm();
  void makeInterComm(MPI_Comm);

  /*! set to given intercomm, and properly set size, root, etc */
  void setTo(MPI_Comm comm);

  /*! do an MPI_Comm_dup, and return duplicated communicator */
  Group dup() const;

  /*! whether the current process/thread is a member of this
    gorup */
  bool containsMe{false};

  /*! communictor for this group. intercommunicator if i'm a
    member of this gorup; else it's an intracommunicator */
  MPI_Comm comm{MPI_COMM_NULL};

  /*! my rank in this group if i'm a member; else set to
    MPI_ROOT */
  int rank{-1};

  /*! size of this group if i'm a member, else size of remote
    group this intracommunicaotr refers to */
  int size{-1};
};

/*! object that handles a message. a message primarily consists of a
  pointer to data; the message itself "owns" this pointer, and
  will delete it once the message itself dies. the message itself
  is reference counted using the std::shared_ptr functionality. */
struct Message
{
  Message() = default;

  /*! create a new message with given amount of bytes in storage */
  Message(size_t size);

  /*! create a new message with given amount of storage, and copy
      memory from the given address to it */
  Message(const void *copyMem, size_t size);

  /*! create a new message (addressed to given comm:rank) with given
      amount of storage, and copy memory from the given address to
      it */
  Message(MPI_Comm comm, int rank, const void *copyMem, size_t size);

  Message(const Message &) = delete;

  Message &operator=(const Message &) = delete;

  /*! destruct message and free allocated memory */
  virtual ~Message();

  bool isValid() const;

  /*! @{ sender/receiver of this message */
  MPI_Comm comm{MPI_COMM_NULL};
  int rank{-1};
  int tag{0};
  /*! @} */

  /*! @{ actual payload of this message */
  rkcommon::byte_t *data{nullptr};
  size_t size{0};
  /*! @} */
  // TODO WILL: Profiling info, when this message started sending
  // or receiving
  std::chrono::high_resolution_clock::time_point started;
};

/*! a message whose payload is owned by the user, and which we do
  NOT delete upon termination */
struct UserMemMessage : public Message
{
  UserMemMessage(void *nonCopyMem, size_t size) : Message()
  {
    data = (rkcommon::byte_t *)nonCopyMem;
    this->size = size;
  }

  /* set data to null to keep the parent from deleting it */
  virtual ~UserMemMessage()
  {
    data = nullptr;
  }
};

//! MPI_COMM_WORLD
extern Group world;

/* The communicator used for the OSPRay workers, may be equivalent to world
 * depending on the launch configuration
 */
extern Group worker;

// Initialize OSPRay's MPI groups, returns false if MPI
// was already initialized. useCommWorld indicates if MPI_COMM_WORLD
// should be set as the world group used for communication. If false,
// it is up to the caller to configure the world group correctly.
bool init(int *ac, const char **av, bool useCommWorld);

bool isManagedObject(OSPDataType type);

inline int workerRank()
{
  return worker.rank;
}

inline int workerSize()
{
  return worker.size;
}

inline int masterRank()
{
  return 0;
}

inline bool IamTheMaster()
{
  return workerRank() == masterRank();
}
} // namespace mpicommon
