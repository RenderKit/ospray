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

#include "../common/common.h"
#include <mpi.h>

namespace macl {

  // IMPI on Windows defines MPI_CALL already, erroneously
#ifdef MPI_CALL
# undef MPI_CALL
#endif
  /*! helper macro that checks the return value of all MPI_xxx(...)
    calls via MPI_CALL(xxx(...)).  */
#define MPI_CALL(a) { int rc = MPI_##a; if (rc != MPI_SUCCESS) throw std::runtime_error("MPI call returned error"); }

  /*! object that handles a message. a message primarily consists of a
      pointer to data; the message itself "owns" this pointer, and
      will delete it once the message itself dies. the message itself
      is reference counted using the std::shared_ptr functionality. */
  struct Message {
    static std::shared_ptr<Message> create(const void *data, size_t size);

    /*! @{ sender/receiver of this message */
    MPI_Comm    comm;
    int         rank;
    /*! @} */
    /*! @{ actual payload of this message */
    void       *data;
    size_t      size;
    /*! @} */

  private:
    Message(const void *data, size_t size);
    ~Message();
};

  
  /*! ABSTRACT communication layer interface */
  struct Comm {
    /*! initialize using an existing MPI communicator */
    static Comm *init(MPI_Comm existingCommunicator);

    /*! lock all MPI communications; do not do any MPI-calls any more
        - whatever may be in fliht at the time - until somebody calls
        'thaw' */
    virtual void freeze();
    /*! re-start all communications. */
    virtual void thaw();
    
    virtual void sendTo(MPI_Comm comm, int rank, const void *data, size_t size);
    

    std::mutex mutex;
    std::vector<Message *> outbox;
    std::vector<Message *> inbox;
    MPI_Comm comm;

  };
  
} // :: macl
