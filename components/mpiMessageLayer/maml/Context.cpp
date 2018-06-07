// ======================================================================== //
// Copyright 2016-2018 Intel Corporation                                    //
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
#include "apps/bench/pico_bench/pico_bench.h"
#include <iostream>
#include <chrono>

#include "ospcommon/memory/malloc.h"
#include "ospcommon/tasking/async.h"
#include "ospcommon/tasking/tasking_system_handle.h"
#include "ospcommon/utility/getEnvVar.h"

using ospcommon::AsyncLoop;
using ospcommon::make_unique;
using ospcommon::tasking::numTaskingThreads;
using ospcommon::utility::getEnvVar;

using namespace std::chrono;

namespace maml {

  /*! the singleton object that handles all the communication */
  std::unique_ptr<Context> Context::singleton = make_unique<Context>();

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
    outbox.push_back(msg);
  }

  void Context::processInboxMessages()
  {
    if (!inbox.empty()) {
      auto incomingMessages = inbox.consume();

      for (auto &message : incomingMessages) {
        auto *handler = handlers[message->comm];
        handler->incoming(message);
      }
    }
  }

  void Context::sendMessagesFromOutbox()
  {
    if (!outbox.empty()) {
      auto outgoingMessages = outbox.consume();

      for (auto &msg : outgoingMessages) {
        MPI_Request request;
        MPI_CALL(Isend(msg->data, msg->size, MPI_BYTE, msg->rank,
                       msg->tag, msg->comm, &request));
        msg->started = high_resolution_clock::now();
        pendingSends.push_back(request);
        sendCache.push_back(std::move(msg));

        madeProgress = true;
      }
    }
  }

  void Context::pollForAndRecieveMessages()
  {
    for (auto &it : handlers) {
      MPI_Comm comm = it.first;

      /* probe if there's something incoming on this handler's comm */
      while (true) {
        int hasIncoming = 0;
        MPI_Status status;
        MPI_CALL(Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG,
                        comm, &hasIncoming, &status));
        if (!hasIncoming) {
          break;
        }
        int size;
        MPI_CALL(Get_count(&status, MPI_BYTE, &size));

        auto msg = std::make_shared<Message>(size);
        msg->rank = status.MPI_SOURCE;
        msg->tag  = status.MPI_TAG;
        msg->comm = comm;

        MPI_Request request;
        MPI_CALL(Irecv(msg->data, size, MPI_BYTE, msg->rank,
                       msg->tag, msg->comm, &request));
        msg->started = high_resolution_clock::now();
        pendingRecvs.push_back(request);
        recvCache.push_back(std::move(msg));

        madeProgress = true;
      }
    }
  }

  void Context::waitOnSomeRequests()
  {
    if (!pendingSends.empty() || !pendingRecvs.empty()) {
      const int totalMessages = pendingSends.size() + pendingRecvs.size();
      int *done = STACK_BUFFER(int, totalMessages);
      MPI_Request *mergedRequests = STACK_BUFFER(MPI_Request, totalMessages);

      for (int i = 0; i < totalMessages; ++i) {
        if (i < pendingSends.size()) {
          mergedRequests[i] = pendingSends[i];
        } else {
          mergedRequests[i] = pendingRecvs[i - pendingSends.size()];
        }
      }

      int numDone = 0;
      if (!madeProgress) {
        MPI_CALL(Waitsome(totalMessages, mergedRequests, &numDone,
                          done, MPI_STATUSES_IGNORE));
      } else {
        MPI_CALL(Testsome(totalMessages, mergedRequests, &numDone,
                          done, MPI_STATUSES_IGNORE));
      }
      auto completed = high_resolution_clock::now();
      madeProgress |= numDone != 0;

      for (int i = 0; i < numDone; ++i) {
        int msgId = done[i];
        if (msgId < pendingSends.size()) {
          pendingSends[msgId] = MPI_REQUEST_NULL;

          auto &msg = sendCache[msgId];
          const auto sendTime = duration_cast<RealMilliseconds>(completed - msg->started);

          {
            std::lock_guard<std::mutex> lock(statsMutex);
            sendTimes.push_back(sendTime);
            sendBandwidth.emplace_back((msg->size / sendTime.count()) * 1e-6f);
          }

          sendCache[msgId].reset();
        } else {
          msgId -= pendingSends.size();
          pendingRecvs[msgId] = MPI_REQUEST_NULL;

          auto &msg = recvCache[msgId];
          const auto recvTime = duration_cast<RealMilliseconds>(completed - msg->started);

          {
            std::lock_guard<std::mutex> lock(statsMutex);
            recvTimes.push_back(recvTime);
            recvBandwidth.emplace_back((msg->size / recvTime.count()) * 1e-6f);
          }

          inbox.push_back(std::move(recvCache[msgId]));
        }
      }

      // Clean up anything we sent
      sendCache.erase(std::remove(sendCache.begin(),
                                  sendCache.end(),
                                  nullptr), sendCache.end());

      pendingSends.erase(std::remove(pendingSends.begin(),
                                     pendingSends.end(),
                                     MPI_REQUEST_NULL), pendingSends.end());

      // Clean up anything we received
      recvCache.erase(std::remove(recvCache.begin(),
                                  recvCache.end(),
                                  nullptr), recvCache.end());

      pendingRecvs.erase(std::remove(pendingRecvs.begin(),
                                     pendingRecvs.end(),
                                     MPI_REQUEST_NULL), pendingRecvs.end());
    }
  }

  void Context::flushRemainingMessages()
  {
    sendMessagesFromOutbox();

    while (!pendingRecvs.empty() && !pendingSends.empty() && !inbox.empty()) {
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
    if (!isRunning()) {
      tasksAreRunning = true;

      auto launchMethod = AsyncLoop::LaunchMethod::AUTO;

      auto MAML_SPAWN_THREADS = getEnvVar<int>("MAML_SPAWN_THREADS");
      if (MAML_SPAWN_THREADS) {
        launchMethod = MAML_SPAWN_THREADS.value() ?
            AsyncLoop::LaunchMethod::THREAD : AsyncLoop::LaunchMethod::TASK;
      }

      if (!sendReceiveThread.get()) {
        sendReceiveThread = make_unique<AsyncLoop>([&](){
          madeProgress = false;
          sendMessagesFromOutbox();
          pollForAndRecieveMessages();

          waitOnSomeRequests();
        }, launchMethod);
      }

      if (!processInboxThread.get()) {
        processInboxThread = make_unique<AsyncLoop>([&](){
          processInboxMessages();
        }, launchMethod);
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
    if they are already in flight */
  void Context::stop()
  {
    tasksAreRunning = false;
    if (sendReceiveThread) {
      sendReceiveThread->stop();
    }
    if (processInboxThread) {
      processInboxThread->stop();
    }
    flushRemainingMessages();
  }

  void Context::logMessageTimings(std::ostream &os)
  {
    std::lock_guard<std::mutex> lock(statsMutex);
    using Stats = pico_bench::Statistics<RealMilliseconds>;
    using SizeStats = pico_bench::Statistics<Bandwidth>;
    if (!sendTimes.empty()) {
      Stats sendStats(sendTimes);
      sendStats.time_suffix = "ms";
      os << "Message send statistics:\n" << sendStats << "\n";

      SizeStats sendBWStats(sendBandwidth);
      sendBWStats.time_suffix = "Gb/s";
      os << "Message send size statistics:\n" << sendBWStats << "\n";
    }

    if (!recvTimes.empty()) {
      Stats recvStats(recvTimes);
      recvStats.time_suffix = "ms";
      os << "Message recv statistics:\n" << recvStats << "\n";

      SizeStats recvBWStats(recvBandwidth);
      recvBWStats.time_suffix = "Gb/s";
      os << "Message recv size statistics:\n" << recvBWStats << "\n";
    }

    sendTimes.clear();
    recvTimes.clear();
    sendBandwidth.clear();
    recvBandwidth.clear();
  }

} // ::maml
