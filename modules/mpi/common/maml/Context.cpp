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

#include "Context.h"
#include <snappy.h>
#include <algorithm>
#include <chrono>
#include <iostream>

#include "ospcommon/memory/malloc.h"
#include "ospcommon/tasking/async.h"
#include "ospcommon/tasking/tasking_system_init.h"
#include "ospcommon/utility/getEnvVar.h"

using ospcommon::byte_t;
using ospcommon::make_unique;
using ospcommon::tasking::AsyncLoop;
using ospcommon::tasking::numTaskingThreads;
using ospcommon::utility::getEnvVar;

using namespace std::chrono;

namespace maml {

  /*! the singleton object that handles all the communication */
  std::unique_ptr<Context> Context::singleton;

  Context::Context(bool enableCompression) : compressMessages(enableCompression)
  {
    auto logging =
        getEnvVar<std::string>("OSPRAY_DP_API_TRACING").value_or("0");
    DETAILED_LOGGING = std::stoi(logging) != 0;
  }

  Context::~Context()
  {
    stop();
  }

  /*! register a new incoing-message handler. if any message comes in
    on the given communicator we'll call this handler */
  void Context::registerHandlerFor(MPI_Comm comm, MessageHandler *handler)
  {
    if (handlers.find(comm) != handlers.end()) {
      std::cerr << CODE_LOCATION
                << ": Warning: handler for this MPI_Comm already installed"
                << std::endl;
    }

    handlers[comm] = handler;

    /*! todo: to avoid race conditions we MAY want to check if there's
        any messages we've already received that would match this
        handler */
  }

  /*! put the given message in the outbox. note that this can be
    done even if the actual sending mechanism is currently
    stopped */
  void Context::send(std::shared_ptr<Message> msg)
  {
    // The message uses malloc/free, so use that instead of new/delete
    if (compressMessages) {
      auto startCompr = high_resolution_clock::now();
      byte_t *compressed =
          (byte_t *)malloc(snappy::MaxCompressedLength(msg->size));
      size_t compressedSize = 0;
      snappy::RawCompress(reinterpret_cast<const char *>(msg->data),
                          msg->size,
                          reinterpret_cast<char *>(compressed),
                          &compressedSize);
      free(msg->data);

      auto endCompr = high_resolution_clock::now();
      if (DETAILED_LOGGING) {
        std::lock_guard<std::mutex> lock(statsMutex);
        compressTimes.push_back(
            duration_cast<RealMilliseconds>(endCompr - startCompr));
        compressedSizes.emplace_back(
            100.0 * (static_cast<double>(compressedSize) / msg->size));
      }

      msg->data = compressed;
      msg->size = compressedSize;
    }

    outbox.push_back(std::move(msg));
  }

  void Context::queueCollective(std::shared_ptr<Collective> col)
  {
    // TODO WILL: auto-compress collectives?
    collectiveOutbox.push_back(std::move(col));
  }

  void Context::processInboxMessages()
  {
    auto incomingMessages = inbox.consume();

    for (auto &msg : incomingMessages) {
      auto *handler = handlers[msg->comm];

      if (compressMessages) {
        auto startCompr = high_resolution_clock::now();
        // Decompress the message before handing it off
        size_t uncompressedSize = 0;
        snappy::GetUncompressedLength(reinterpret_cast<const char *>(msg->data),
                                      msg->size,
                                      &uncompressedSize);
        byte_t *uncompressed = (byte_t *)malloc(uncompressedSize);
        snappy::RawUncompress(reinterpret_cast<const char *>(msg->data),
                              msg->size,
                              reinterpret_cast<char *>(uncompressed));
        free(msg->data);
        const size_t compressedSize = msg->size;

        auto endCompr = high_resolution_clock::now();
        if (DETAILED_LOGGING) {
          std::lock_guard<std::mutex> lock(statsMutex);
          decompressTimes.push_back(
              duration_cast<RealMilliseconds>(endCompr - startCompr));
          compressedSizes.emplace_back(
              100.0 * (static_cast<double>(compressedSize) / uncompressedSize));
        }

        msg->data = uncompressed;
        msg->size = uncompressedSize;
      }

      handler->incoming(msg);
    }
  }

  void Context::sendMessagesFromOutbox()
  {
    auto outgoingMessages    = outbox.consume();
    auto outgoingCollectives = collectiveOutbox.consume();

    for (auto &msg : outgoingMessages) {
      MPI_Request request;

      int rank = 0;
      MPI_CALL(Comm_rank(msg->comm, &rank));
      // Don't send to ourself, just forward to the inbox directly
      if (rank == msg->rank) {
        inbox.push_back(std::move(msg));
      } else {
        MPI_CALL(Isend(msg->data,
                       msg->size,
                       MPI_BYTE,
                       msg->rank,
                       msg->tag,
                       msg->comm,
                       &request));
        msg->started = high_resolution_clock::now();

        pendingSends.push_back(request);
        sendCache.push_back(std::move(msg));
      }
    }

    for (auto &col : outgoingCollectives) {
      col->start();
      pendingCollectives.push_back(col);
    }
  }

  void Context::pollForAndRecieveMessages()
  {
    for (auto &it : handlers) {
      MPI_Comm comm = it.first;

      /* probe if there's something incoming on this handler's comm */
      int hasIncoming = 0;
      MPI_Status status;
      MPI_CALL(
          Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &hasIncoming, &status));

      if (hasIncoming) {
        int size;
        MPI_CALL(Get_count(&status, MPI_BYTE, &size));

        auto msg  = std::make_shared<Message>(size);
        msg->rank = status.MPI_SOURCE;
        msg->tag  = status.MPI_TAG;
        msg->comm = comm;

        MPI_Request request;
        MPI_CALL(Irecv(msg->data,
                       size,
                       MPI_BYTE,
                       msg->rank,
                       msg->tag,
                       msg->comm,
                       &request));

        msg->started = high_resolution_clock::now();

        pendingRecvs.push_back(request);
        recvCache.push_back(std::move(msg));
      }
    }
  }

  void Context::waitOnSomeRequests()
  {
    if (!pendingSends.empty() || !pendingRecvs.empty()) {
      const size_t totalMessages  = pendingSends.size() + pendingRecvs.size();
      int *done                   = STACK_BUFFER(int, totalMessages);
      MPI_Request *mergedRequests = STACK_BUFFER(MPI_Request, totalMessages);

      for (size_t i = 0; i < totalMessages; ++i) {
        if (i < pendingSends.size()) {
          mergedRequests[i] = pendingSends[i];
        } else {
          mergedRequests[i] = pendingRecvs[i - pendingSends.size()];
        }
      }

      int numDone = 0;
      MPI_CALL(Testsome(totalMessages,
                        mergedRequests,
                        &numDone,
                        done,
                        MPI_STATUSES_IGNORE));
      auto completed = high_resolution_clock::now();

      for (int i = 0; i < numDone; ++i) {
        size_t msgId = done[i];
        if (msgId < pendingSends.size()) {
          if (DETAILED_LOGGING) {
            std::lock_guard<std::mutex> lock(statsMutex);
            Message *msg = sendCache[msgId].get();
            sendTimes.push_back(
                duration_cast<RealMilliseconds>(completed - msg->started));
          }

          pendingSends[msgId] = MPI_REQUEST_NULL;
          sendCache[msgId]    = nullptr;
        } else {
          msgId -= pendingSends.size();

          if (DETAILED_LOGGING) {
            std::lock_guard<std::mutex> lock(statsMutex);
            Message *msg = recvCache[msgId].get();
            recvTimes.push_back(
                duration_cast<RealMilliseconds>(completed - msg->started));
          }

          inbox.push_back(std::move(recvCache[msgId]));

          pendingRecvs[msgId] = MPI_REQUEST_NULL;
          recvCache[msgId]    = nullptr;
        }
      }

      // Clean up anything we sent
      sendCache.erase(std::remove(sendCache.begin(), sendCache.end(), nullptr),
                      sendCache.end());

      pendingSends.erase(
          std::remove(
              pendingSends.begin(), pendingSends.end(), MPI_REQUEST_NULL),
          pendingSends.end());

      // Clean up anything we received
      recvCache.erase(std::remove(recvCache.begin(), recvCache.end(), nullptr),
                      recvCache.end());

      pendingRecvs.erase(
          std::remove(
              pendingRecvs.begin(), pendingRecvs.end(), MPI_REQUEST_NULL),
          pendingRecvs.end());
    }
    if (!pendingCollectives.empty()) {
      pendingCollectives.erase(
          std::remove_if(pendingCollectives.begin(),
                         pendingCollectives.end(),
                         [](const std::shared_ptr<Collective> &col) {
                           return col->finished();
                         }),
          pendingCollectives.end());
    }
  }

  void Context::flushRemainingMessages()
  {
    while (!pendingRecvs.empty() || !pendingSends.empty() ||
           !pendingCollectives.empty() || !inbox.empty() || !outbox.empty()) {
      sendMessagesFromOutbox();
      pollForAndRecieveMessages();
      waitOnSomeRequests();
      processInboxMessages();
    }
  }

  /*! start the service; from this point on maml is free to use MPI
    calls to send/receive messages; if your MPI library is not
    thread safe the app should _not_ do any MPI calls until 'stop()'
    has been called */
  void Context::start()
  {
    std::lock_guard<std::mutex> lock(tasksMutex);
    if (!isRunning()) {
      tasksAreRunning = true;

      auto launchMethod = AsyncLoop::LaunchMethod::AUTO;

      auto MAML_SPAWN_THREADS = getEnvVar<int>("MAML_SPAWN_THREADS");
      if (MAML_SPAWN_THREADS) {
        launchMethod = MAML_SPAWN_THREADS.value()
                           ? AsyncLoop::LaunchMethod::THREAD
                           : AsyncLoop::LaunchMethod::TASK;
      }

      if (!sendReceiveThread.get()) {
        sendReceiveThread = make_unique<AsyncLoop>(
            [&]() {
              sendMessagesFromOutbox();
              pollForAndRecieveMessages();
              waitOnSomeRequests();
            },
            launchMethod);
      }

      if (!processInboxThread.get()) {
        processInboxThread = make_unique<AsyncLoop>(
            [&]() { processInboxMessages(); }, launchMethod);
      }

      sendReceiveThread->start();
      processInboxThread->start();
    }
  }

  bool Context::isRunning() const
  {
    return tasksAreRunning;
  }

  /*! stops the maml layer; maml will no longer perform any MPI calls;
    if the mpi layer is not thread safe the app is then free to use
    MPI calls of its own, but it should not expect that this node
    receives any more messages (until the next 'start()' call) even
    if they are already in fligh
    WILL: Don't actually stop, for reasons described above flush messages
  */
  void Context::stop()
  {
    std::lock_guard<std::mutex> lock(tasksMutex);
    if (tasksAreRunning) {
      quitThreads = true;
      if (sendReceiveThread) {
        sendReceiveThread->stop();
      }
      if (processInboxThread) {
        processInboxThread->stop();
      }

      tasksAreRunning = false;
      flushRemainingMessages();
    }
  }

  void Context::logMessageTimings(std::ostream & /*os*/)
  {
    if (!DETAILED_LOGGING) {
      return;
    }
#if 0  // can't depend on pico_bench here from apps/ directory
    std::lock_guard<std::mutex> lock(statsMutex);
    using Stats = pico_bench::Statistics<RealMilliseconds>;
    using CompressedStats = pico_bench::Statistics<CompressionPercent>;
    if (!sendTimes.empty()) {
      Stats sendStats(sendTimes);
      sendStats.time_suffix = "ms";
      os << "Message send statistics:\n" << sendStats << "\n";
    }

    if (!recvTimes.empty()) {
      Stats recvStats(recvTimes);
      recvStats.time_suffix = "ms";
      os << "Message recv statistics:\n" << recvStats << "\n";
    }

    if (!compressTimes.empty()) {
      Stats stats(compressTimes);
      stats.time_suffix = "ms";
      os << "Compression statistics:\n" << stats << "\n";
    }

    if (!decompressTimes.empty()) {
      Stats stats(decompressTimes);
      stats.time_suffix = "ms";
      os << "Decompression statistics:\n" << stats << "\n";
    }

    if (!compressedSizes.empty()) {
      CompressedStats stats(compressedSizes);
      stats.time_suffix = "%";
      os << "Compressed Size statistics:\n" << stats << "\n";
    }
#endif

    sendTimes.clear();
    recvTimes.clear();
    compressTimes.clear();
    decompressTimes.clear();
    compressedSizes.clear();
  }

}  // namespace maml
