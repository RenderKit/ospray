// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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

// ospray
#include "mpiCommon/MPICommon.h"

namespace ospray {
  namespace mpi {

    /*! abstraction for an MPI-group-to-MPI-group 'service' that one group opens,
      and that (an)other MPI group(s) can then 'connect' to.

      Server creates a service with a suggested TCP port via a call to
      
      std::shared_ptr<Service> = Service::create(mpiComm,suggestedPort).

      rank 0 of the passed mpi comm will open a TCP/IP port on the
      suggeted port number (or any other one if that one is
      unsuccessful). The operation is a joint operation among all
      ranks in the passed group.

      Once that service is 'create'ed, rank 0 on the server group can
      query the created port using 'service->getPortString()' (which
      is of the form <hostname>:<portNo>. Clients can 'connect' to
      this port, and the server can 'service->accept()' connection
      requests.

      On the client side, a connection is initiated using 

      std::shared_ptr<Service::Connection> = Service::connect(mpiComm,serviceAddress);

      with serviceAddress of the form <hostnameOfRank0>:<portNo>. Again,
      this is a collaborative call that all ranks in the provided
      mpiComm have to partake in.

      For both server and client, the Service::Connection returned
      from connect()/accept() contains an MPI group that contains an
      intercommunicator between those two groups.
    */
    struct Service {
      
      struct Connection : public mpi::Group {
        void close();
        virtual ~Connection();
      };

      // -------------------------------------------------------
      // client side interface
      // -------------------------------------------------------
      
      /*! have the provided mpi communicator create a connection to
          the service on 'serverAddr'. If that service hasn't been
          created (or for any other reason we cannot create a
          connection) we will throw an exception, else we return a
          handle to a connection that contains an intercomm to the MPI
          comm providing this service */
      static std::shared_ptr<Connection> connect(MPI_Comm comm, const std::string &serverAddr);

      // -------------------------------------------------------
      // server side interface
      // -------------------------------------------------------
      
      /*! create a new service on given port (on host of rank 0), and
          return a handle t this service. A client can
          Service::connect() to this port, and we can
          service->accept() these connection requests */
      static std::shared_ptr<Service> create(MPI_Comm comm, int suggestedPort=-1);
      
      /*! accept a new connection request. this will block until we
          either receive a timeout, or a client connection has been
          established */
      std::shared_ptr<Connection> accept();

      /*! return a 'address' string that the clients can connect
          to. typically, this is of the form "<hostname>:<portNo>",
          where hostname is the name of the machine that runs rank 0,
          and portNo is the TCP/IP socket that this service is
          published on */
      std::string getServiceAddress() const;

      mpi::Group getComm() const { return comm; }

      /*! destroy and close */
      virtual ~Service();
      
      /*! close the created socket and mpi group */
      void close();
      
    // private:
      int listenSocket { -1 };
      std::string serviceAddress;
      /*! a 'dup' of the group that created the service */
      mpi::Group  comm;
    };
    
  } // ::mpi
} // ::ospray
