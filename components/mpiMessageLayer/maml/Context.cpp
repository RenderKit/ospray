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

namespace maml {

  /*! the singleton object that handles all the communication */
  Context *Context::singleton = NULL;

  Context::Context()
    : canDoMPICalls(false), flushed(true)
  {
    mpiThreadHandle   = std::thread([this](){ mpiThread(); });
    inboxThreadHandle = std::thread([this](){ inboxThread(); });
  }
  
  /*! register a new incoing-message handler. if any message comes in
    on the given communicator we'll call this handler */
  void Context::registerHandlerFor(MPI_Comm comm, MessageHandler *handler)
  {
    std::lock_guard<std::mutex> lock(handlersMutex);
    if (handlers.find(comm) != handlers.end())
      std::cout << __PRETTY_FUNCTION__
                << ": Warning: handler for this MPI_Comm already installed" << std::endl;
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
    assert(msg);

    std::lock_guard<std::mutex> lock(outboxMutex);
    std::lock_guard<std::mutex> flushLock(flushMutex);
    
    outbox.push_back(msg);
    flushed = false;
  }
    

  void Context::inboxThread()
  {
    while (1) {
      std::vector<std::shared_ptr<Message> > execList;

      // fetch the list of messages that need to be executed
      {
        std::unique_lock<std::mutex> lock(inboxMutex);
        if (inbox.empty())
          inboxCondition.wait(lock, [this]{return !inbox.empty();});
        execList = inbox;
        // std::cout << "found " << inbox.size() << " messages in inbox!" << std::endl;
        inbox.clear();
      }

      for (int i=0;i<execList.size();i++) {
        MessageHandler *handler = NULL;
        { std::lock_guard<std::mutex> lock(handlersMutex);
          handler = handlers[execList[i]->comm];
          assert(handler);
        }

        handler->incoming(execList[i]);
        execList[i] = NULL;
      }
    }
  }
  
  void Context::mpiThread()
  {
    /*! list of messages we are currently in the process of receiving
      from the MPI layer - ie, we know those messages exist, but
      they have NOT yet been fully received */
    std::vector<std::shared_ptr<Message> > currentlyReceiving;
    
    /*! list of messages we are currently in the process of sending
      via the MPI layer - ie, we have already triggered an MPI_Isend,
      but have not verified that the message has been received yet */
    std::vector<std::shared_ptr<Message> > currentlySending;
    
    while (1) {
      bool anyDoneSend = false;
      bool anyDoneRecv = false;
      /*! the messages we have finihsed receiving this round */
      std::vector<std::shared_ptr<Message> > inbox;
      
      { /* do all MPI commands under the mpi lock */
        std::unique_lock<std::mutex> lock(canDoMPIMutex);
        canDoMPICondition.wait(lock, [this]{return this->canDoMPICalls;});
        
        /* check if there's any outgoing messages ... and trigger
           their sending */
        { std::lock_guard<std::mutex> lock(outboxMutex);
          if (!outbox.empty()) {
            for (int i=0;i<outbox.size();i++) {
              std::shared_ptr<Message> msg = outbox[i];
              assert(msg);
              assert(msg->data);
              MPI_CALL(Isend(msg->data,msg->size,MPI_BYTE,msg->rank,msg->tag,msg->comm,&msg->request));
              currentlySending.push_back(msg);
              // std::cout << "got new out message - starting to send and pushing to send queue ..." << std::endl;
            }
            outbox.clear();
          }
        }
        
        /* check if there's any new incoming message(s) ... and trigger receiving for any new ones */
        {
          std::lock_guard<std::mutex> lock(handlersMutex);
          /* check each message handler (we're not receiveing anything
             unless we have a handler */
          for (auto it=handlers.begin(); it != handlers.end(); it++) {
            MessageHandler *handler = it->second;
            MPI_Comm        comm    = it->first;

            /* probe if there's something incoming on this handler's comm */
            int             hasIncoming = 0;
            MPI_Status status;
            MPI_CALL(Iprobe(MPI_ANY_SOURCE,MPI_ANY_TAG,comm,&hasIncoming,&status));
            if (hasIncoming) {
              
              int size;
              MPI_CALL(Get_count(&status,MPI_BYTE,&size));
              
              std::shared_ptr<Message> msg = std::shared_ptr<Message>(new Message(size));
              msg->rank = status.MPI_SOURCE;
              msg->tag  = status.MPI_TAG;
              msg->comm = comm;
              MPI_CALL(Irecv(msg->data,size,MPI_BYTE,msg->rank,msg->tag,msg->comm,&msg->request));
              currentlyReceiving.push_back(msg);
            }
          }
        }
        
        /* wait for ANY of the currently incoming our outgoing
           messages to be done sending/receiving */
        if (!(currentlySending.empty() && currentlyReceiving.empty())) {
          // create list of all currently active requests:
          size_t numSendRequests = currentlySending.size();
          size_t numRecvRequests = currentlyReceiving.size();
          size_t numRequests     = numSendRequests+numRecvRequests;
          MPI_Request activeRequest[numRequests];
          // - add all ongoing send requests
          for (int i=0;i<currentlySending.size();i++)
            activeRequest[i] = currentlySending[i]->request;
          // - add all ongoing recv requests
          for (int i=0;i<currentlyReceiving.size();i++)
            activeRequest[numSendRequests+i] = currentlyReceiving[i]->request;

          // wait for at least one of those to complete
          int done[numRequests];
          int numDone = 0;
          // printf("waitsome(%i) send=%i recv=%i %i\n",rank,numSendRequests,numRecvRequests,numRequests);
          if (numRecvRequests == 0) {
            MPI_CALL(Testsome(numRequests,activeRequest,&numDone,done,MPI_STATUSES_IGNORE));
          } else {
            MPI_CALL(Waitsome(numRequests,activeRequest,&numDone,done,MPI_STATUSES_IGNORE));
          }
          for (int i=0;i<numDone;i++) {
            int requestID = done[i];
            if (requestID >= numSendRequests) {
              // this was a recv request
              std::shared_ptr<Message> msg = currentlyReceiving[requestID-numSendRequests];
              currentlyReceiving[requestID-numSendRequests] = NULL;
              inbox.push_back(msg);
              anyDoneRecv = true;
            } else {
              // this was a send request that just finished
              currentlySending[requestID] = NULL;
              anyDoneSend = true;
            }
          }
        }
      } /* end of scope ofr MPI lock - we can now operate on the
           messages we've received (and update out local state), but
           from here on out we're no longer do any MPI calls (until
           the next round) !!!! */
      
      /* purge the send queue if any sends completed */
      if (anyDoneSend) {
        std::vector<std::shared_ptr<Message> > stillNotDone;
        for (auto it=currentlySending.begin();it!=currentlySending.end();it++)
          if (*it) stillNotDone.push_back(*it);
        currentlySending = stillNotDone;
      }
      static long lastSendSize = -1;
      if (lastSendSize != currentlySending.size()) {
        lastSendSize = currentlySending.size();
      }
      
      /* purge the recv queue if any sends completed */
      if (anyDoneRecv) {
        std::vector<std::shared_ptr<Message> > stillNotDone;
        for (auto it=currentlyReceiving.begin();it!=currentlyReceiving.end();it++)
          if (*it) stillNotDone.push_back(*it);
        currentlyReceiving = stillNotDone;
      }

      if (anyDoneRecv) {
        std::lock_guard<std::mutex> lock(inboxMutex);
        for (int i=0;i<inbox.size();i++)
          this->inbox.push_back(inbox[i]);
        inboxCondition.notify_one();
      }

      if (currentlyReceiving.empty() && currentlySending.empty()) {
        std::lock_guard<std::mutex> outboxLock(outboxMutex);
        if (outbox.empty()) {
          std::lock_guard<std::mutex> flushLock(flushMutex);
          flushed = true;
          flushCondition.notify_one();
        }
      }
      
    }
  }

  /*! make sure all outgoing messages get sent... */
  void Context::flush()
  {
    std::unique_lock<std::mutex> lock(flushMutex);

    // if (!flushed)
    //   { std::lock_guard<std::mutex> outboxLock(outboxMutex);
        // std::cout << "#maml: flushing (" << outbox.size() << " messages in outbox)!" << std::endl;
      // }
    flushCondition.wait(lock, [this]{return flushed;});
    // std::cout << "#maml: flushed..." << std::endl;
  }
  
  /*! start the service; from this point on maml is free to use MPI
    calls to send/receive messages; if your MPI library is not
    thread safe the app should _not_ do any MPI calls until 'stop()'
    has been called */
  void Context::start()
  {
    std::lock_guard<std::mutex> m1(canDoMPIMutex);
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
    std::lock_guard<std::mutex> m1(canDoMPIMutex);
    canDoMPICalls = false;
  }

} // ::maml
