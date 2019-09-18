// ======================================================================== //
// Copyright 2016-2019 Intel Corporation                                    //
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

#include <mpi.h>
#include <future>
#include <memory>
#include "MPICommon.h"
#include "ospcommon/utility/ArrayView.h"

namespace mpicommon {
// Convenient wrappers over the collectives //

/* Start an asynchronous bcast and return the future to wait on
 * for completion. The caller owns the passed buffer, and must keep it
 * valid until the future completes.
 */
std::future<void *> OSPRAY_MPI_INTERFACE bcast(
    void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm);

// TODO: Maybe we can just send a unique ptr or move only types
// NOTE: std::future dtor blocks, so for async sending we need to hang
// on to the futures and check on them
std::future<void *> OSPRAY_MPI_INTERFACE bcast(
    std::shared_ptr<ospcommon::utility::ArrayView<uint8_t>> &buffer,
    int count,
    MPI_Datatype datatype,
    int root,
    MPI_Comm comm);

/* Start an asynchronous barrier and return the future to wait on for
 * completion of the barrier
 */
std::future<void> OSPRAY_MPI_INTERFACE barrier(MPI_Comm comm);

/* Start an asynchronous gather and return the future to wait on for
 * completion of the gather. The called owns both the send and recv buffers,
 * and must keep them valid until the future completes. The pointer returned
 * in the future is to the receive buffer.
 */
std::future<void *> OSPRAY_MPI_INTERFACE gather(const void *sendBuffer,
    int sendCount,
    MPI_Datatype sendType,
    void *recvBuffer,
    int recvCount,
    MPI_Datatype recvType,
    int root,
    MPI_Comm comm);

/* Start an asynchronous gatherv and return the future to wait on for
 * completion of the gather. The called owns both the send and recv buffers,
 * and must keep them valid until the future completes. The pointer returned
 * in the future is to the receive buffer. The recvCounts and offsets
 * vectors are copied into the struct.
 */
std::future<void *> OSPRAY_MPI_INTERFACE gatherv(const void *sendBuffer,
    int sendCount,
    MPI_Datatype sendType,
    void *recvBuffer,
    const std::vector<int> &recvCounts,
    const std::vector<int> &recvOffsets,
    MPI_Datatype recvType,
    int root,
    MPI_Comm comm);

/* Start an asynchronously run reduce. The send/recv buffers are owned
 * by the caller and must be kept valid until the future is set, indicating
 * completion of the reduction.
 */
std::future<void *> OSPRAY_MPI_INTERFACE reduce(const void *sendBuffer,
    void *recvBuffer,
    int count,
    MPI_Datatype datatype,
    MPI_Op operation,
    int root,
    MPI_Comm comm);

/* Start an asynchronously run send. The buffer is owned by
 * the caller and must be kept valid until the future is set, indicating
 * completion of the send.
 */
std::future<void *> OSPRAY_MPI_INTERFACE send(void *buffer,
    int count,
    MPI_Datatype datatype,
    int source,
    int tag,
    MPI_Comm comm);

/* Start an asynchronously run recv. The buffer is owned by
 * the caller and must be kept valid until the future is set, indicating
 * completion of the recv.
 */
std::future<void *> OSPRAY_MPI_INTERFACE recv(void *buffer,
    int count,
    MPI_Datatype datatype,
    int source,
    int tag,
    MPI_Comm comm);

// An asynchronously executed collective operation which can be run
// on the MPI messaging layer
class OSPRAY_MPI_INTERFACE Collective
{
 public:
  Collective(MPI_Comm comm);
  virtual ~Collective() = default;

  // Start the collective
  virtual void start() = 0;
  // Check if the collective is done and notify the child classes onFinish
  virtual bool finished();

 protected:
  virtual void onFinish() = 0;

  MPI_Comm comm;
  MPI_Request request;
};

class OSPRAY_MPI_INTERFACE Barrier : public Collective
{
 public:
  Barrier(MPI_Comm comm);
  // Get the future to wait on completion of this barrier
  std::future<void> future();
  void start() override;

 protected:
  void onFinish() override;

 private:
  std::promise<void> result;
};

class OSPRAY_MPI_INTERFACE Bcast : public Collective
{
 public:
  /* Construct an asynchronously run broadcast. The buffer is owned by
   * the caller and must be kept valid until the future is set, indicating
   * completion of the broadcast.
   */
  Bcast(std::shared_ptr<ospcommon::utility::ArrayView<uint8_t>> buffer,
      int count,
      MPI_Datatype datatype,
      int root,
      MPI_Comm comm);

  // Get the future which will receive the result of this bcast
  std::future<void *> future();
  void start() override;
  bool finished() override;

 protected:
  void onFinish() override;

 private:
  std::shared_ptr<ospcommon::utility::ArrayView<uint8_t>> buffer;
  int count;
  int typeSize;
  MPI_Datatype datatype;
  int root;
  std::promise<void *> result;
  std::vector<MPI_Request> requests;
};

class OSPRAY_MPI_INTERFACE Gather : public Collective
{
 public:
  /* Construct an asynchronously run gather. The send/recv buffers are owned
   * by the caller and must be kept valid until the future is set, indicating
   * completion of the gather.
   */
  Gather(const void *sendBuffer,
      int sendCount,
      MPI_Datatype sendType,
      void *recvBuffer,
      int recvCount,
      MPI_Datatype recvType,
      int root,
      MPI_Comm comm);

  /* Get the future which will receive the result of this gather.
   * The returned pointer will point to the recvBuffer containing the
   * received data.
   */
  std::future<void *> future();
  void start() override;

 protected:
  void onFinish() override;

 private:
  const void *sendBuffer;
  int sendCount;
  MPI_Datatype sendType;

  void *recvBuffer;
  int recvCount;
  MPI_Datatype recvType;

  int root;
  std::promise<void *> result;
};

class OSPRAY_MPI_INTERFACE Gatherv : public Collective
{
 public:
  /* Construct an asynchronously run gatherv. The send/recv buffers are owned
   * by the caller and must be kept valid until the future is set, indicating
   * completion of the gatherv. The recvOffsets and counts vectors are copied
   * into the struct.
   */
  Gatherv(const void *sendBuffer,
      int sendCount,
      MPI_Datatype sendType,
      void *recvBuffer,
      const std::vector<int> &recvCounts,
      const std::vector<int> &recvOffsets,
      MPI_Datatype recvType,
      int root,
      MPI_Comm comm);

  /* Get the future which will receive the result of this gatherv.
   * The returned pointer will point to the recvBuffer containing the
   * received data.
   */
  std::future<void *> future();
  void start() override;

 protected:
  void onFinish() override;

 private:
  const void *sendBuffer;
  int sendCount;
  MPI_Datatype sendType;

  void *recvBuffer;
  std::vector<int> recvCounts;
  std::vector<int> recvOffsets;
  MPI_Datatype recvType;

  int root;
  std::promise<void *> result;
};

class OSPRAY_MPI_INTERFACE Reduce : public Collective
{
 public:
  /* Construct an asynchronously run reduce. The send/recv buffers are owned
   * by the caller and must be kept valid until the future is set, indicating
   * completion of the reduction.
   */
  Reduce(const void *sendBuffer,
      void *recvBuffer,
      int count,
      MPI_Datatype datatype,
      MPI_Op operation,
      int root,
      MPI_Comm comm);

  /* Get the future which will receive the result of this reduction.
   * The returned pointer will point to the recvBuffer containing the
   * result of the reduction.
   */
  std::future<void *> future();
  void start() override;

 protected:
  void onFinish() override;

 private:
  const void *sendBuffer;
  void *recvBuffer;
  int count;
  MPI_Datatype datatype;
  MPI_Op operation;
  int root;

  std::promise<void *> result;
};

/* Send/recv are not really collectives, but are separate from the typical
 * fire and forget style of messaging that the mpicommon::Message uses.
 * With these we actually want the ability to wait for a specific send to
 * finish, and a specific recv from a process to be completed. With the
 * fire and forget messaging layer we don't care about this and just queue
 * stuff up and recieve whatever is coming to us.
 */
class OSPRAY_MPI_INTERFACE Send : public Collective
{
 public:
  /* Construct an asynchronously run send. The buffer is owned by
   * the caller and must be kept valid until the future is set, indicating
   * completion of the send.
   */
  Send(void *buffer,
      int count,
      MPI_Datatype datatype,
      int dest,
      int tag,
      MPI_Comm comm);

  /* Get the future which will return when this send is done, returns the
   * pointer to the sent buffer
   */
  std::future<void *> future();
  void start() override;

 protected:
  void onFinish() override;

 private:
  void *buffer;
  int count;
  MPI_Datatype datatype;
  int dest;
  int tag;
  std::promise<void *> result;
};

class OSPRAY_MPI_INTERFACE Recv : public Collective
{
 public:
  /* Construct an asynchronously run recv. The buffer is owned by
   * the caller and must be kept valid until the future is set, indicating
   * completion of the recv.
   */
  Recv(void *buffer,
      int count,
      MPI_Datatype datatype,
      int source,
      int tag,
      MPI_Comm comm);

  /* Get the future which will return when this recv is done, returns the
   * pointer to the buffer containing the recv'd data
   */
  std::future<void *> future();
  void start() override;

 protected:
  void onFinish() override;

 private:
  void *buffer;
  int count;
  MPI_Datatype datatype;
  int source;
  int tag;
  std::promise<void *> result;
};

} // namespace mpicommon
