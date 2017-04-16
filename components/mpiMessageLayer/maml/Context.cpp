// ************************************************************************** //
// Copyright 2016 Ingo Wald                                                   //
//                                                                            //
// Licensed under the Apache License, Version 2.0 (the "License");            //
// you may not use this file except in compliance with the License.           //
// You may obtain a copy of the License at                                    //
//                                                                            //
// http://www.apache.org/licenses/LICENSE-2.0                                 //
//                                                                            //
// Unless required by applicable law or agreed to in writing, software        //
// distributed under the License is distributed on an "AS IS" BASIS,          //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   //
// See the License for the specific language governing permissions and        //
// limitations under the License.                                             //
// ************************************************************************** //

#include "Context.h"
#include <iostream>

using ospcommon::make_unique;

namespace maml {

  /*! the singleton object that handles all the communication */
  std::unique_ptr<Context> Context::singleton = make_unique<Context>();

  Context::Context()
  {
    mpiSendRecvThread = std::thread([this](){ mpiSendAndRecieveThread(); });
    inboxProcThread   = std::thread([this](){ processInboxThread();      });
  }

  Context::~Context()
  {
    threadsRunning = false;
    start();// wake up mpiSendRecvThread so it can be shutdown
    mpiSendRecvThread.join();
    inboxProcThread.join();
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

  void Context::processInboxThread()
  {
    while(threadsRunning) {
      if (!inbox.empty()) {
        auto incomingMessages = inbox.consume();

        for (auto &message : incomingMessages) {
          auto *handler = handlers[message->comm];
          handler->incoming(message);
        }
      }
    }
  }

  void Context::mpiSendAndRecieveThread()
  {
    while(true) {
      std::unique_lock<std::mutex> lock(canDoMPIMutex);
      canDoMPICondition.wait(lock, [&]{ return canDoMPICalls; });

      if (!threadsRunning)
        return;

      sendAndRecieveThreadActive = true;

      sendMessagesFromOutbox();
      pollForAndRecieveMessages();

      waitOnSomeSendRequests();
      waitOnSomeRecvRequests();

      sendAndRecieveThreadActive = false;
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
        pendingSends.push_back(request);
        sendCache.push_back(std::move(msg));
      }
    }
  }

  void Context::pollForAndRecieveMessages()
  {
    for (auto &it : handlers) {
      MPI_Comm comm = it.first;

      /* probe if there's something incoming on this handler's comm */
      int hasIncoming = 0;
      MPI_Status status;
      MPI_CALL(Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG,
                      comm, &hasIncoming, &status));

      if (hasIncoming) {
        int size;
        MPI_CALL(Get_count(&status, MPI_BYTE, &size));

        auto msg = std::make_shared<Message>(size);
        msg->rank = status.MPI_SOURCE;
        msg->tag  = status.MPI_TAG;
        msg->comm = comm;

        MPI_Request request;
        MPI_CALL(Irecv(msg->data, size, MPI_BYTE, msg->rank,
                       msg->tag, msg->comm, &request));
        pendingRecvs.push_back(request);
        recvCache.push_back(std::move(msg));
      }
    }
  }

  void Context::waitOnSomeSendRequests()
  {
    if (!pendingSends.empty()) {
      int numDone = 0;
      int *done = STACK_BUFFER(int, pendingSends.size());

      MPI_CALL(Testsome(pendingSends.size(), pendingSends.data(), &numDone,
                        done, MPI_STATUSES_IGNORE));

      for (int i = 0; i < numDone; ++i) {
        int pendingSendCompletedIndex = done[i];
        pendingSends[pendingSendCompletedIndex] = nullptr;
        sendCache[pendingSendCompletedIndex]    = nullptr;
      }

      sendCache.erase(std::remove(sendCache.begin(),
                                  sendCache.end(),
                                  nullptr), sendCache.end());

      pendingSends.erase(std::remove(pendingSends.begin(),
                                     pendingSends.end(),
                                     nullptr), pendingSends.end());
    }
  }

  void Context::waitOnSomeRecvRequests()
  {
    if (!pendingRecvs.empty()) {
      int numDone = 0;
      int *done = STACK_BUFFER(int, pendingRecvs.size());

      MPI_CALL(Waitsome(pendingRecvs.size(), pendingRecvs.data(), &numDone,
                        done, MPI_STATUSES_IGNORE));

      for (int i = 0; i < numDone; ++i) {
        int pendingRecvCompletedIndex = done[i];
        pendingRecvs[pendingRecvCompletedIndex] = nullptr;
        inbox.push_back(std::move(recvCache[pendingRecvCompletedIndex]));
      }

      recvCache.erase(std::remove(recvCache.begin(),
                                  recvCache.end(),
                                  nullptr), recvCache.end());

      pendingRecvs.erase(std::remove(pendingRecvs.begin(),
                                     pendingRecvs.end(),
                                     nullptr), pendingRecvs.end());
    }
  }

  void Context::flushRemainingMessages()
  {
    sendMessagesFromOutbox();

    while (!pendingRecvs.empty())
      waitOnSomeRecvRequests();

    while (!pendingSends.empty())
      waitOnSomeSendRequests();
  }

  /*! start the service; from this point on maml is free to use MPI
    calls to send/receive messages; if your MPI library is not
    thread safe the app should _not_ do any MPI calls until 'stop()'
    has been called */
  void Context::start()
  {
    canDoMPICalls = true;
    canDoMPICondition.notify_one();
  }

  /*! stops the maml layer; maml will no longer perform any MPI calls;
    if the mpi layer is not thread safe the app is then free to use
    MPI calls of its own, but it should not expect that this node
    receives any more messages (until the next 'start()' call) even
    if they are already in flight */
  void Context::stop()
  {
    canDoMPICalls = false;
    std::unique_lock<std::mutex> lock(canDoMPIMutex);
    canDoMPICondition.wait(lock, [&]{ return !sendAndRecieveThreadActive; });

    flushRemainingMessages();
  }

} // ::maml
