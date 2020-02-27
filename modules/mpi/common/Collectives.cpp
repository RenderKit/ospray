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

#include "Collectives.h"
#include <algorithm>
#include <cmath>
#include <memory>
#include <numeric>
#include "maml/maml.h"

namespace mpicommon {
using namespace ospcommon;

std::future<void *> bcast(
    void *buf, size_t count, MPI_Datatype datatype, int root, MPI_Comm comm)
{
  int typeSize = 0;
  MPI_Type_size(datatype, &typeSize);
  auto view = std::make_shared<utility::ArrayView<uint8_t>>(
      static_cast<uint8_t *>(buf), count * typeSize);
  auto col = std::make_shared<Bcast>(view, count, datatype, root, comm);
  maml::queueCollective(col);
  return col->future();
}

std::future<void *> OSPRAY_MPI_INTERFACE bcast(
    std::shared_ptr<ospcommon::utility::ArrayView<uint8_t>> &buffer,
    size_t count,
    MPI_Datatype datatype,
    int root,
    MPI_Comm comm)
{
  auto col = std::make_shared<Bcast>(buffer, count, datatype, root, comm);
  maml::queueCollective(col);
  return col->future();
}

std::future<void> barrier(MPI_Comm comm)
{
  auto col = std::make_shared<Barrier>(comm);
  maml::queueCollective(col);
  return col->future();
}

std::future<void *> gather(const void *sendBuffer,
    int sendCount,
    MPI_Datatype sendType,
    void *recvBuffer,
    int recvCount,
    MPI_Datatype recvType,
    int root,
    MPI_Comm comm)
{
  auto col = std::make_shared<Gather>(sendBuffer,
      sendCount,
      sendType,
      recvBuffer,
      recvCount,
      recvType,
      root,
      comm);
  maml::queueCollective(col);
  return col->future();
}

std::future<void *> gatherv(const void *sendBuffer,
    int sendCount,
    MPI_Datatype sendType,
    void *recvBuffer,
    const std::vector<int> &recvCounts,
    const std::vector<int> &recvOffsets,
    MPI_Datatype recvType,
    int root,
    MPI_Comm comm)
{
  auto col = std::make_shared<Gatherv>(sendBuffer,
      sendCount,
      sendType,
      recvBuffer,
      recvCounts,
      recvOffsets,
      recvType,
      root,
      comm);
  maml::queueCollective(col);
  return col->future();
}

std::future<void *> send(void *buffer,
    int count,
    MPI_Datatype datatype,
    int dest,
    int tag,
    MPI_Comm comm)
{
  auto col = std::make_shared<Send>(buffer, count, datatype, dest, tag, comm);
  maml::queueCollective(col);
  return col->future();
}

std::future<void *> recv(void *buffer,
    int count,
    MPI_Datatype datatype,
    int source,
    int tag,
    MPI_Comm comm)
{
  auto col = std::make_shared<Recv>(buffer, count, datatype, source, tag, comm);
  maml::queueCollective(col);
  return col->future();
}

std::future<void *> reduce(const void *sendBuffer,
    void *recvBuffer,
    int count,
    MPI_Datatype datatype,
    MPI_Op operation,
    int root,
    MPI_Comm comm)
{
  auto col = std::make_shared<Reduce>(
      sendBuffer, recvBuffer, count, datatype, operation, root, comm);
  maml::queueCollective(col);
  return col->future();
}

Collective::Collective(MPI_Comm comm) : comm(comm), request(MPI_REQUEST_NULL) {}

bool Collective::finished()
{
  int done = 0;
  MPI_CALL(Test(&request, &done, MPI_STATUS_IGNORE));
  if (done) {
    onFinish();
  }
  return done;
}

Barrier::Barrier(MPI_Comm comm) : Collective(comm) {}

std::future<void> Barrier::future()
{
  return result.get_future();
}

void Barrier::start()
{
  MPI_CALL(Ibarrier(comm, &request));
}

void Barrier::onFinish()
{
  result.set_value();
}

Bcast::Bcast(std::shared_ptr<ospcommon::utility::ArrayView<uint8_t>> buffer,
    size_t count,
    MPI_Datatype datatype,
    int root,
    MPI_Comm comm)
    : Collective(comm),
      buffer(buffer),
      count(count),
      datatype(datatype),
      root(root)
{
  MPI_Type_size(datatype, &typeSize);
}

std::future<void *> Bcast::future()
{
  return result.get_future();
}

void Bcast::start()
{
  // 1GB as the max bcast size
  const static size_t MAX_BCAST_SIZE = 1e9;
  size_t remaining = count;
  // TODO: This is Rust's slice::chunks iterator
  uint8_t *iter = buffer->begin();
  do {
    MPI_Request req;
    const size_t toSend = std::min(MAX_BCAST_SIZE, remaining);
    MPI_CALL(Ibcast(iter, toSend, datatype, root, comm, &req));
    requests.push_back(req);

    iter += toSend * typeSize;
    remaining -= toSend;
  } while (iter != buffer->end());
}

bool Bcast::finished()
{
  const int ndone = std::accumulate(
      requests.begin(), requests.end(), 0, [](const int &n, MPI_Request &r) {
        int d = 0;
        MPI_CALL(Test(&r, &d, MPI_STATUS_IGNORE));
        if (d) {
          return n + 1;
        }
        return n;
      });

  if (ndone == requests.size()) {
    onFinish();
  }
  return ndone == requests.size();
}

void Bcast::onFinish()
{
  result.set_value(static_cast<void *>(buffer->data()));
}

Gather::Gather(const void *sendBuffer,
    int sendCount,
    MPI_Datatype sendType,
    void *recvBuffer,
    int recvCount,
    MPI_Datatype recvType,
    int root,
    MPI_Comm comm)
    : Collective(comm),
      sendBuffer(sendBuffer),
      sendCount(sendCount),
      sendType(sendType),
      recvBuffer(recvBuffer),
      recvCount(recvCount),
      recvType(recvType),
      root(root)
{}

std::future<void *> Gather::future()
{
  return result.get_future();
}

void Gather::start()
{
  MPI_CALL(Igather(sendBuffer,
      sendCount,
      sendType,
      recvBuffer,
      recvCount,
      recvType,
      root,
      comm,
      &request));
}

void Gather::onFinish()
{
  result.set_value(recvBuffer);
}

Gatherv::Gatherv(const void *sendBuffer,
    int sendCount,
    MPI_Datatype sendType,
    void *recvBuffer,
    const std::vector<int> &recvCounts,
    const std::vector<int> &recvOffsets,
    MPI_Datatype recvType,
    int root,
    MPI_Comm comm)
    : Collective(comm),
      sendBuffer(sendBuffer),
      sendCount(sendCount),
      sendType(sendType),
      recvBuffer(recvBuffer),
      recvCounts(recvCounts),
      recvOffsets(recvOffsets),
      recvType(recvType),
      root(root)
{}

std::future<void *> Gatherv::future()
{
  return result.get_future();
}

void Gatherv::start()
{
  MPI_CALL(Igatherv(sendBuffer,
      sendCount,
      sendType,
      recvBuffer,
      recvCounts.data(),
      recvOffsets.data(),
      recvType,
      root,
      comm,
      &request));
}

void Gatherv::onFinish()
{
  result.set_value(recvBuffer);
}

Reduce::Reduce(const void *sendBuffer,
    void *recvBuffer,
    int count,
    MPI_Datatype datatype,
    MPI_Op operation,
    int root,
    MPI_Comm comm)
    : Collective(comm),
      sendBuffer(sendBuffer),
      recvBuffer(recvBuffer),
      count(count),
      datatype(datatype),
      operation(operation),
      root(root)
{}

std::future<void *> Reduce::future()
{
  return result.get_future();
}

void Reduce::start()
{
  MPI_CALL(Ireduce(sendBuffer,
      recvBuffer,
      count,
      datatype,
      operation,
      root,
      comm,
      &request));
}

void Reduce::onFinish()
{
  result.set_value(recvBuffer);
}

Send::Send(void *buffer,
    int count,
    MPI_Datatype datatype,
    int dest,
    int tag,
    MPI_Comm comm)
    : Collective(comm),
      buffer(buffer),
      count(count),
      datatype(datatype),
      dest(dest),
      tag(tag)
{}

std::future<void *> Send::future()
{
  return result.get_future();
}
void Send::start()
{
  MPI_CALL(Isend(buffer, count, datatype, dest, tag, comm, &request));
}

void Send::onFinish()
{
  result.set_value(buffer);
}

Recv::Recv(void *buffer,
    int count,
    MPI_Datatype datatype,
    int source,
    int tag,
    MPI_Comm comm)
    : Collective(comm),
      buffer(buffer),
      count(count),
      datatype(datatype),
      source(source),
      tag(tag)
{}

std::future<void *> Recv::future()
{
  return result.get_future();
}

void Recv::start()
{
  MPI_CALL(Irecv(buffer, count, datatype, source, tag, comm, &request));
}

void Recv::onFinish()
{
  result.set_value(buffer);
}
} // namespace mpicommon
