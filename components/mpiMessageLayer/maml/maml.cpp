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

#include "maml.h"
#include "Context.h"

namespace maml {

  std::mutex messageMutex;
  
  /*! create a new message with given amount of bytes in storage */
  Message::Message(size_t size)
    : data(NULL), size(size),
      comm(MPI_COMM_NULL), rank(-1), tag(0)
  {
    //new unsigned char [size]
    assert(size > 0);

    // { std::lock_guard<std::mutex> lock(messageMutex);
    //   static size_t sumAlloc = 0;
    //   sumAlloc += size;
    //   PRINT(size);
    //   PRINT(sumAlloc);
      data = (unsigned char *)malloc(size);
      // PRINT((int*)data);
    // }
    
    assert(data);
  }
  
  /*! create a new message with given amount of storage, and copy
    memory from the given address to it */
  Message::Message(const void *copyMem, size_t size)
    : data(NULL), size(size),
      comm(MPI_COMM_NULL), rank(-1), tag(0)
  {
    assert(copyMem);

    assert(size > 0);
    data = (unsigned char *) malloc(size); //new unsigned char [size]
    assert(data);
      
    memcpy(data,copyMem,size);
  }

    /*! create a new message (addressed to given comm:rank) with given
        amount of storage, and copy memory from the given address to
        it */
  Message::Message(MPI_Comm comm, int rank,
                   const void *copyMem, size_t size)
    : data(NULL), size(size),
      comm(comm), rank(rank), tag(0)
  {
    assert(copyMem);
    assert( size > 0);

    //    data = new unsigned char [size];
    data = (unsigned char *)malloc(size);
    assert(data);
    memcpy(data,copyMem,size);
  }
    
  /*! destruct message and free allocated memory */
  Message::~Message() {
    if (data) {
      // delete[] data;
      free(data); //printf("currently NOT deleting the messages!\n");
    }
  }
  
  
  /*! initialize the maml layer. must be called before doing any call
      below, but should only be called once. note this assuems that
      MPI is already initialized; it will use the existing MPI
      layer */
  void init(int &ac, char **&av)
  {
    int initialized;
    MPI_CALL(Initialized(&initialized));
    if (!initialized)
      MAML_THROW("MPI not initialized");

    if (Context::singleton)
      MAML_THROW("MAML layer _already_ initialized");
    
    Context::singleton = new Context;
    assert(Context::singleton);
  }

  /*! register a new incoing-message handler. if any message comes in
    on the given communicator we'll call this handler */
  void registerHandlerFor(MPI_Comm comm, MessageHandler *handler)
  {
    if (!Context::singleton)
      MAML_THROW("MAML layer not initialized?!");

    Context::singleton->registerHandlerFor(comm,handler);
  }

  /*! start the service; from this point on maml is free to use MPI
      calls to send/receive messages; if your MPI library is not
      thread safe the app should _not_ do any MPI calls until 'stop()'
      has been called */
  void start()
  {
    if (!Context::singleton)
      MAML_THROW("MAML layer not initialized?!");

    Context::singleton->start();
  }

  /*! stops the maml layer; maml will no longer perform any MPI calls;
      if the mpi layer is not thread safe the app is then free to use
      MPI calls of its own, but it should not expect that this node
      receives any more messages (until the next 'start()' call) even
      if they are already in flight */
  void stop()
  {
    if (!Context::singleton)
      MAML_THROW("MAML layer not initialized?!");

    Context::singleton->stop();
  }

  /*! send given messsage to given comm:rank. Once this function has
      called maml has full ownership of this message, and the user may
      no longer access it (becuase maml may delete it at any time) */
  void send(std::shared_ptr<Message> msg)
  {
    assert(msg->comm != MPI_COMM_NULL);
    assert(msg->rank >= 0);
    if (!Context::singleton)
      MAML_THROW("MAML layer not initialized?!");
    Context::singleton->send(std::shared_ptr<Message>(msg));
  }
  
  // void send(Message *msg)
  // {
  //   assert(msg);
  //   send(std::shared_ptr<Message>(msg));
  // }
  
  
  /*! send given messsage to given comm:rank. Once this function has
      called maml has full ownership of this message, and the user may
      no longer access it (becuase maml may delete it at any time) */
  // void sendTo(MPI_Comm comm, int rank, Message *msg)
  // {
  //   assert(msg);
  //   msg->rank = rank;
  //   msg->comm = comm;
  //   msg->tag  = 0;
  //   send(msg);
  // }

  /*! send given messsage to given comm:rank. Once this function has
    called maml has full ownership of this message, and the user may
    no longer access it (becuase maml may delete it at any time) */
  void sendTo(MPI_Comm comm, int rank, std::shared_ptr<Message> msg)
  {
    assert(msg);
    assert(rank >= 0);
    msg->rank = rank;
    msg->comm = comm;
    msg->tag  = 0;
    send(msg);
  }

  /*! make sure all outgoing messages get sent... */
  void flush()
  {
    if (!Context::singleton)
      MAML_THROW("MAML layer not initialized?!");
    Context::singleton->flush();
  }
  
  /*! close down and clean exit. NOT YET IMPLEMENTED. */
  void finalize()
  {
    std::cout << "#maml: warning: maml::finalize() not yet implemented" << std::endl;
  }
  

  
} // ::maml

