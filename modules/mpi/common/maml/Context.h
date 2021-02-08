// Copyright 2016-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <future>
#include <map>
#include <mutex>
#include <vector>
#include "common/Collectives.h"
#include "maml.h"
#include "rkcommon/containers/TransactionalBuffer.h"
#include "rkcommon/tasking/AsyncLoop.h"

namespace maml {

/*! the singleton object that handles all the communication */
struct OSPRAY_MPI_INTERFACE Context
{
  Context(bool enableCompression = false);
  ~Context();

  static std::unique_ptr<Context> singleton;

  /*! register a new incoing-message handler. if any message comes in
    on the given communicator we'll call this handler */
  void registerHandlerFor(MPI_Comm comm, MessageHandler *handler);

  bool isRunning() const;

  /*! put the given message in the outbox. note that this can be
      done even if the actual sending mechanism is currently
      stopped */
  void send(std::shared_ptr<Message> msg);

  /*! queue the given collective to be executed */
  void queueCollective(std::shared_ptr<Collective> col);

  // WILL: Logging
  void logMessageTimings(std::ostream &os);

  /*! start the service; from this point on maml is free to use MPI
    calls to send/receive messages; if your MPI library is not
    thread safe the app should _not_ do any MPI calls until 'stop()'
    has been called */
  void start();

  /*! stops the maml layer; maml will no longer perform any MPI calls;
    if the mpi layer is not thread safe the app is then free to use
    MPI calls of its own, but it should not expect that this node
    receives any more messages (until the next 'start()' call) even
    if they are already in flight */
  void stop();

 private:
  // Helper functions //

  /*! the thread that executes messages that the receiver thread
      put into the inbox */
  void processInboxTask();

  void processInboxMessages();

  /*! the thread (function) that executes all MPI commands to
      send/receive messages via MPI.

      Some notes:

      - it only looks for incoming messages on communicators for
      which a handler has been specified. if you don't add a handler
      to a comm, nothing will ever get received from this comm (you
      may still send on it, though!)

      - messages to be sent are retrieved from 'outbox'; messages that
        are received get put to 'inbox' and the 'inboxcondition' gets
        triggered. it's another thread's job to execute those
        messages
  */
  void sendMessagesFromOutbox();
  void pollForAndRecieveMessages();
  void waitOnSomeRequests();

  void flushRemainingMessages();

  // Data members //

  rkcommon::containers::TransactionalBuffer<std::shared_ptr<Message>> inbox;
  rkcommon::containers::TransactionalBuffer<std::shared_ptr<Message>> outbox;
  rkcommon::containers::TransactionalBuffer<std::shared_ptr<Collective>>
      collectiveOutbox;

  // NOTE(jda) - sendCache/pendingSends MUST correspond with each other by
  //             their index in their respective vectors...
  std::vector<std::shared_ptr<Message>> sendCache;
  std::vector<MPI_Request> pendingSends;

  // NOTE(jda) - recvCache/pendingRecvs MUST correspond with each other by
  //             their index in their respective vectors...
  std::vector<std::shared_ptr<Message>> recvCache;
  std::vector<MPI_Request> pendingRecvs;

  std::vector<std::shared_ptr<Collective>> pendingCollectives;

  std::map<MPI_Comm, MessageHandler *> handlers;

  bool useTaskingSystem{true};
  bool compressMessages{false};

  // NOTE(jda) - these are only used when _not_ using the tasking system...
  std::mutex tasksMutex;
  bool tasksAreRunning{false};
  // std::thread sendReceiveThread, processInboxThread;
  std::atomic<bool> quitThreads{false};
  std::unique_ptr<rkcommon::tasking::AsyncLoop> sendReceiveThread;
  std::unique_ptr<rkcommon::tasking::AsyncLoop> processInboxThread;

  std::mutex statsMutex;
  using CompressionPercent = std::chrono::duration<double>;

  using RealMilliseconds = std::chrono::duration<double, std::milli>;
  std::vector<CompressionPercent> compressedSizes;
  std::vector<RealMilliseconds> sendTimes, recvTimes, compressTimes,
      decompressTimes;

  bool DETAILED_LOGGING{false};
};

} // namespace maml
