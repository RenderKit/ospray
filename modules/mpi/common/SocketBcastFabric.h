// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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

#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>
#include "MPICommon.h"
#include "ospcommon/containers/TransactionalBuffer.h"
#include "ospcommon/networking/Fabric.h"
#include "ospcommon/networking/Socket.h"
#include "ospcommon/tasking/AsyncLoop.h"

namespace mpicommon {

/*! A writer which broadcasts from one rank to the others over sockets */
struct OSPRAY_MPI_INTERFACE SocketWriterFabric : public networking::Fabric
{
  /*! Connect to the clients who will received the broadcasts by querying
   * the client rank info from the info server at host:port */
  SocketWriterFabric(const std::string &hostname, const uint16_t port);
  ~SocketWriterFabric() override;

  SocketWriterFabric(const SocketWriterFabric &) = delete;
  SocketWriterFabric &operator=(const SocketWriterFabric &) = delete;

  void sendBcast(std::shared_ptr<utility::AbstractArray<uint8_t>> buf) override;

  void recvBcast(utility::AbstractArray<uint8_t> &buf) override;

  void send(
      std::shared_ptr<utility::AbstractArray<uint8_t>> buf, int rank) override;

  void recv(utility::AbstractArray<uint8_t> &buf, int rank) override;

 private:
  void sendThreadLoop();

  struct Message
  {
    std::shared_ptr<utility::AbstractArray<uint8_t>> buf;
    // -1 for all ranks, or a rank id for a single rank
    int ranks;

    Message(std::shared_ptr<utility::AbstractArray<uint8_t>> &buf, int ranks);
  };

  std::vector<ospcommon::socket_t> sockets;
  std::unique_ptr<ospcommon::tasking::AsyncLoop> sendThread;
  ospcommon::containers::TransactionalBuffer<Message> outbox;
};

/*! A reader which reads data broadcast out from a root process
 * over sockets */
struct OSPRAY_MPI_INTERFACE SocketReaderFabric : public networking::Fabric
{
  /*! Setup the connection by listening for an incoming
    connection on the desired port. The MPI group is used on the
    recieving end to set up the info server so we can send back
    the root's info to the other ranks */
  SocketReaderFabric(const Group &parentGroup, const uint16_t port);
  ~SocketReaderFabric();

  SocketReaderFabric(const SocketReaderFabric &) = delete;
  SocketReaderFabric &operator=(const SocketReaderFabric &) = delete;

  void sendBcast(std::shared_ptr<utility::AbstractArray<uint8_t>> buf) override;

  void recvBcast(utility::AbstractArray<uint8_t> &buf) override;

  void send(
      std::shared_ptr<utility::AbstractArray<uint8_t>> buf, int rank) override;

  void recv(utility::AbstractArray<uint8_t> &buf, int rank) override;

 private:
  void sendThreadLoop();

  Group group;
  ospcommon::socket_t socket;
  std::unique_ptr<ospcommon::tasking::AsyncLoop> sendThread;
  ospcommon::containers::TransactionalBuffer<
      std::shared_ptr<utility::AbstractArray<uint8_t>>>
      outbox;
};
} // namespace mpicommon
