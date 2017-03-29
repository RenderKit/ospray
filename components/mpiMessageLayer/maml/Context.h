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

#pragma once

#include "maml.h"
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace maml {

  /*! the singleton object that handles all the communication */
  struct Context {
    Context();
    
    static Context *singleton;

    /*! register a new incoing-message handler. if any message comes in
      on the given communicator we'll call this handler */
    void registerHandlerFor(MPI_Comm comm, MessageHandler *handler);

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

    /*! put the given message in the outbox. note that this can be
        done even if the actual sending mechanism is currently
        stopped */
    void send(std::shared_ptr<Message> msg);

    /*! the thread (function) that executes all MPI commands to
        send/receive messages via MPI. 

        Some notes: 

        - this thread does MPI calls (only!) between calls of start()
        and end(). unless you cal start(), nothing will ever get sent
        or received.

        - it only looks for incoming messages on communicators for
        which a handler has been specified. if you don't add a handler
        to a comm, nothing will ever get received from this comm (you
        may still send on it, though!)

        - messages to be sent are retried from 'outbox'; messages that
          are received get put to 'inbox' and the 'iboxcondition' gets
          triggered. it's another thread's job to execute those
          messages
    */
    void mpiThread();

    /*! the thread that executes messages that the receiveer thread
        put into the inbox */
    void inboxThread();

    /*! make sure all outgoing messages get sent... */
    void flush();

    bool                    canDoMPICalls;
    std::mutex              canDoMPIMutex;
    std::condition_variable canDoMPICondition;
    
    std::thread mpiThreadHandle;
    std::thread inboxThreadHandle;

    std::mutex                          handlersMutex;
    std::map<MPI_Comm,MessageHandler *> handlers;

    /*! new messsages that still need to get sent */
    std::mutex                             outboxMutex;
    std::vector<std::shared_ptr<Message> > outbox;

    /*! new messsages that have been received but not yet executed */
    std::mutex                             inboxMutex;
    std::condition_variable                inboxCondition;
    std::vector<std::shared_ptr<Message> > inbox;

    /*! used to execute a flush: this condition gets triggered when
      all messages in the outbox have been (fully) sent */
    bool                    flushed;
    std::mutex              flushMutex;
    std::condition_variable flushCondition;
  };
  
} // ::maml
