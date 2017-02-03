// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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

#pragma once

#include "../MPICommon.h"

namespace ospray {
  namespace mpi {

    typedef int32_t int32;
    typedef uint32_t uint32;
    typedef int64_t int64;
    typedef uint64_t uint64;
    
    namespace async {
      struct Message
      {
        Address     addr;
        void       *ptr;
        int32       size;
        MPI_Request request; //! request for MPI_Test
        int         done;    //! done flag for MPI_Test
        MPI_Status  status;  //! status for MPI_Test
      };

      /*! a 'async::Consumer is any class that can handle incoming
          messages. how it handles those isn't defined, other than
          that the consumer is supposed to 'free' the message after it
          is done processing (the comm layer itself will never free
          it!), and that it should strive to process this message as
          quickly as it possibly can (using a separate worker
          thread(s) if need be */
      struct Consumer
      {
        virtual void process(const Address &source, void *message, int32 size) = 0;
      };
    
      struct OSPRAY_MPI_INTERFACE Group : public mpi::Group
      {
        //! message consumer (may be NULL)

        Group(// const std::string &name, 
              MPI_Comm comm, 
              Consumer *consumer, int32 tag = MPI_ANY_TAG);
        // virtual std::string toString() { return "mpi::async::Group("+name+")"; };

        //! consumer to handle this group's incoming messags
        Consumer   *consumer;
        //! tag to be used for asynchronous messaging
        int32      tag;
      };

      //! abstraction - internally used - implement the messaging MPI
      struct OSPRAY_MPI_INTERFACE AsyncMessagingImpl
      {
        virtual void init() = 0;
        virtual void shutdown() = 0;
        virtual Group *createGroup(MPI_Comm comm,
                                   Consumer *consumer,
                                   int32 tag = MPI_ANY_TAG) = 0;
        virtual void send(const Address &dest, void *msgPtr, int32 msgSize) = 0;

        static AsyncMessagingImpl *global;
      };

      /*! @{ The actual asynchronous messaging API */
      OSPRAY_MPI_INTERFACE Group *createGroup(MPI_Comm comm,
                                              Consumer *consumer,
                                              int32 tag = MPI_ANY_TAG);
      OSPRAY_MPI_INTERFACE void shutdown();

      /*! send a asynchronous message to the address specified. the
          'group' in said address HAS to be a group created via
          'async::Group' */
      OSPRAY_MPI_INTERFACE void send(const Address &dest, void *msgPtr, int32 msgSize);
      /*! @} */

    } // ::ospray::mpi::async


  } // ::ospray::mpi
} // ::ospray
