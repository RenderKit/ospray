// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>
#include "MPICommon.h"
#include "rkcommon/containers/TransactionalBuffer.h"
#include "rkcommon/networking/Fabric.h"
#include "rkcommon/networking/Socket.h"
#include "rkcommon/tasking/AsyncLoop.h"

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

  void flushBcastSends() override;

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

  std::vector<rkcommon::socket_t> sockets;
  std::unique_ptr<rkcommon::tasking::AsyncLoop> sendThread;
  rkcommon::containers::TransactionalBuffer<Message> outbox;

  std::mutex mutex;
  bool bcasts_in_outbox;
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

  void flushBcastSends() override;

  void recvBcast(utility::AbstractArray<uint8_t> &buf) override;

  void send(
      std::shared_ptr<utility::AbstractArray<uint8_t>> buf, int rank) override;

  void recv(utility::AbstractArray<uint8_t> &buf, int rank) override;

 private:
  void sendThreadLoop();

  Group group;
  rkcommon::socket_t socket;
  std::unique_ptr<rkcommon::tasking::AsyncLoop> sendThread;
  rkcommon::containers::TransactionalBuffer<
      std::shared_ptr<utility::AbstractArray<uint8_t>>>
      outbox;
};
} // namespace mpicommon
