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

#include "SocketBcastFabric.h"
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>
#include "Collectives.h"
#include "ospcommon/networking/Socket.h"

namespace mpicommon {

using namespace ospcommon;
using ospcommon::tasking::AsyncLoop;

SocketWriterFabric::Message::Message(
    std::shared_ptr<utility::AbstractArray<uint8_t>> &buf, int ranks)
    : buf(buf), ranks(ranks)
{}

/* The socket bcast fabric setup initially listens on the readers, which
 * the writer connects to. The writer then sends back info about the port
 * which it's listening on, and lets the readers connect back to it. This
 * is useful for systems like stampede2 where the compute nodes aren't
 * visible from the outside network but can make connections to the outside.
 */

SocketWriterFabric::SocketWriterFabric(
    const std::string &hostname, const uint16_t port)
{
  // Listen on a port the readers can connect back to, to setup the fabric
  socket_t listenSock = ospcommon::listen(0);

  // Connect to the client info server and get back the number of readers
  uint32_t numClients = 0;
  {
    socket_t infoServer;
    try {
      infoServer = ospcommon::connect(hostname.c_str(), port);
    } catch (const std::runtime_error &e) {
      std::cerr << "Failed to connect to " << hostname << ":" << port << "\n";
      throw e;
    }

    ospcommon::read(infoServer, &numClients, sizeof(uint32_t));

    const uint16_t listenPort = getListenPort(listenSock);
    const std::string myIP = getIP(infoServer);

    ospcommon::write(infoServer, &listenPort, sizeof(uint16_t));
    const uint64_t ipLen = myIP.size();
    ospcommon::write(infoServer, &ipLen, sizeof(uint64_t));
    ospcommon::write(infoServer, &myIP[0], myIP.size());
  }

  sockets.resize(numClients, OSP_INVALID_SOCKET);
  size_t numConnected = 0;
  do {
    // TODO Will: Check the behavior of the backlog with a large number
    // of incoming connections
    socket_t c = ospcommon::accept(listenSock);
    uint32_t clientRank = 0;
    ospcommon::read(c, &clientRank, sizeof(uint32_t));
    sockets[clientRank] = c;
    ++numConnected;
  } while (numConnected < numClients);

  ospcommon::close(listenSock);

  sendThread = ospcommon::make_unique<AsyncLoop>([&]() { sendThreadLoop(); });
  sendThread->start();
}

SocketWriterFabric::~SocketWriterFabric()
{
  sendThread->stop();
  for (auto &s : sockets) {
    ospcommon::close(s);
  }
}

void SocketWriterFabric::sendBcast(
    std::shared_ptr<utility::AbstractArray<uint8_t>> buf)
{
  outbox.push_back(Message(buf, -1));
}

void SocketWriterFabric::flushBcastSends()
{
  do {
    std::lock_guard<std::mutex> lock(mutex);
    if (outbox.empty() && !bcasts_in_outbox) {
      return;
    }
  } while (true);
}

void SocketWriterFabric::recvBcast(utility::AbstractArray<uint8_t> &buf)
{
  throw std::runtime_error("Cannot recvBcast on socket writer!");
}

void SocketWriterFabric::send(
    std::shared_ptr<utility::AbstractArray<uint8_t>> buf, int rank)
{
  outbox.push_back(Message(buf, rank));
}

void SocketWriterFabric::recv(utility::AbstractArray<uint8_t> &buf, int rank)
{
  ospcommon::read(sockets[rank], buf.data(), buf.size());
}

void SocketWriterFabric::sendThreadLoop()
{
  std::vector<Message> msg;
  {
    std::lock_guard<std::mutex> lock(mutex);
    msg = outbox.consume();
    bcasts_in_outbox = false;
    for (auto &m : msg) {
      if (m.ranks == -1) {
        bcasts_in_outbox = true;
        break;
      }
    }
  }

  for (auto &m : msg) {
    if (m.ranks == -1) {
      for (auto &s : sockets) {
        ospcommon::write(s, m.buf->data(), m.buf->size());
      }
    } else {
      ospcommon::write(sockets[m.ranks], m.buf->data(), m.buf->size());
    }
  }
}

SocketReaderFabric::SocketReaderFabric(
    const Group &parentGroup, const uint16_t port)
    : group(parentGroup.dup())
{
  // Rank 0 of the readers listens on the port for the writer to connect,
  // and receives its host and port info to broadcast out to the other clients
  uint16_t bcastWriterPort = 0;
  std::string bcastWriterHost;
  if (group.rank == 0) {
    socket_t listenSock = ospcommon::listen(port);

    if (port == 0) {
      std::cout << "#osp: Listening on port " << getListenPort(listenSock)
                << "\n"
                << std::flush;
    }

    socket_t client = ospcommon::accept(listenSock);

    // Send back the number of clients
    uint32_t numClients = group.size;
    ospcommon::write(client, &numClients, sizeof(uint32_t));

    ospcommon::read(client, &bcastWriterPort, sizeof(uint16_t));
    uint64_t clientStrLen = 0;
    ospcommon::read(client, &clientStrLen, sizeof(uint64_t));
    bcastWriterHost = std::string(clientStrLen, ' ');
    ospcommon::read(client, &bcastWriterHost[0], clientStrLen);
  }

  bcast(&bcastWriterPort, sizeof(uint16_t), MPI_BYTE, 0, group.comm).wait();
  uint64_t bcastStrLen = bcastWriterHost.size();
  bcast(&bcastStrLen, sizeof(uint64_t), MPI_BYTE, 0, group.comm).wait();
  if (group.rank != 0) {
    bcastWriterHost = std::string(bcastStrLen, ' ');
  }
  bcast(&bcastWriterHost[0], bcastStrLen, MPI_BYTE, 0, group.comm).wait();

  // TODO: Will we have ECONNREFUSED if we have a lot of clients and exceed
  // the listen buffer?
  socket = ospcommon::connect(bcastWriterHost.c_str(), bcastWriterPort);
  const uint32_t myRank = group.rank;
  ospcommon::write(socket, &myRank, sizeof(uint32_t));

  sendThread = ospcommon::make_unique<AsyncLoop>([&]() { sendThreadLoop(); });
  sendThread->start();
}

SocketReaderFabric::~SocketReaderFabric()
{
  sendThread->stop();
  if (socket != ospcommon::OSP_INVALID_SOCKET) {
    ospcommon::close(socket);
  }
}

void SocketReaderFabric::sendBcast(
    std::shared_ptr<utility::AbstractArray<uint8_t>> buf)
{
  throw std::runtime_error("Reader fabric can't send bcast!");
}

void SocketReaderFabric::flushBcastSends() {}

void SocketReaderFabric::recvBcast(utility::AbstractArray<uint8_t> &buf)
{
  ospcommon::read(socket, buf.data(), buf.size());
}

void SocketReaderFabric::send(
    std::shared_ptr<utility::AbstractArray<uint8_t>> buf, int rank)
{
  if (rank != 0)
    throw std::runtime_error("Reader fabric can only send to bcast root");

  outbox.push_back(buf);
}

void SocketReaderFabric::recv(utility::AbstractArray<uint8_t> &buf, int rank)
{
  if (rank != 0)
    throw std::runtime_error("Reader fabric can only recv from bcast root");

  ospcommon::read(socket, buf.data(), buf.size());
}

void SocketReaderFabric::sendThreadLoop()
{
  auto msg = outbox.consume();
  for (auto &m : msg) {
    ospcommon::write(socket, m->data(), m->size());
  }
}
} // namespace mpicommon
