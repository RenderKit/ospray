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

#include "mpiCommon/MPICommon.h"

#define MAML_THROW(a) \
  throw std::runtime_error("in "+std::string(__PRETTY_FUNCTION__)+" : "+std::string(a))

namespace maml {

  struct Context;
  
  /*! object that handles a message. a message primarily consists of a
    pointer to data; the message itself "owns" this pointer, and
    will delete it once the message itself dies. the message itself
    is reference counted using the std::shared_ptr functionality. */
  struct Message
  {
    /*! create a new message with given amount of bytes in storage */
    Message(size_t size);
    
    /*! create a new message with given amount of storage, and copy
        memory from the given address to it */
    Message(const void *copyMem, size_t size);

    /*! create a new message (addressed to given comm:rank) with given
        amount of storage, and copy memory from the given address to
        it */
    Message(MPI_Comm comm, int rank,
            const void *copyMem, size_t size);
    
    /*! destruct message and free allocated memory */
    virtual ~Message();

    bool isValid() const;

    /*! @{ sender/receiver of this message */
    MPI_Comm  comm {MPI_COMM_NULL};
    int       rank {-1};
    int       tag  { 0};
    /*! @} */

    /*! @{ actual payload of this message */
    ospcommon::byte_t   *data {nullptr};
    size_t               size {0};
    /*! @} */
    
  private:
    /*! the request for sending/receiving this message */
    MPI_Request request;
    
    friend class Context;
  };
  
  /*! a message whose payload is owned by the user, and which we do
    NOT delete upon termination */
  struct UserMemMessage : public Message
  {
    UserMemMessage(const void *nonCopyMem, size_t size)
      : Message(nonCopyMem,size)
    {}

    /* set data to null to keep the parent from deleting it */
    virtual ~UserMemMessage()
    { data = nullptr; }
  };

  /*! initialize the maml layer. must be called before doing any call
      below, but should only be called once. note this assuems that
      MPI is already initialized; it will use the existing MPI
      layer */
  void init(int &ac, char **&av);

  /*! abstraction for an object that can receive messages. handlers
      get associated with MPI_Comm's, and get called automatically
      every time such a message comes in. maml receives the message,
      then passed it to the handler, from which point it is owned by -
      and the respsonsibility of - the handler */
  struct MessageHandler
  {
    virtual void incoming(const std::shared_ptr<Message> &message) = 0;
  };
  
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

  /*! close down and clean exit. NOT YET IMPLEMENTED. */
  void finalize();
  
  /*! schedule the given message to be send to the comm:rank indicated
      in this message. comm and rank have to be a valid address. Once
      this function has called maml has full ownership of this
      message, and the user may no longer access it (becuase maml may
      delete it at any time). note this message will not be sent
      immediately if the mpi sending is stopped; it will, however, be
      placed in the outbox to be sent at the next possible
      opportunity. */
  // void send(Message *msg);
  void send(std::shared_ptr<Message> msg);

  /*! make sure all outgoing messages get sent... */
  void flush();
  
  /*! schedule the given message to be send to the given
      comm:rank. comm and rank have to be a valid address. Once this
      function has called maml has full ownership of this message, and
      the user may no longer access it (becuase maml may delete it at
      any time). note this message will not be sent immediately if the
      mpi sending is stopped; it will, however, be placed in the
      outbox to be sent at the next possible opportunity. 

      WARNING: calling flush does NOT guarantee that there's no more
      messages coming in to this node: it does mean that all the
      messages we've already MPI_Probe'd will get finished receiving,
      but it will NOT guarantee that another node(s) aren't just
      sending out some new message(s) that simply haven't even STARTED
      arriving on this node, yet!!!
  */
  void sendTo(MPI_Comm comm, int rank, std::shared_ptr<Message> msg);

} // ::maml
