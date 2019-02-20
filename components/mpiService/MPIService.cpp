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

// ospray
#include "MPIService.h"
// socket stuff
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h> 
#include <string.h>
#include <assert.h>
#include <sstream>

#define INVALID_SOCKET -1

namespace ospray {
  namespace mpi {

    Service::Connection::~Connection()
    {
      close();
    }
    
    void Service::Connection::close()
    {
      if (comm != MPI_COMM_NULL) {
        MPI_CALL(Comm_disconnect(&this->comm));
        setTo(MPI_COMM_NULL);
      }
    }
    
    Service::~Service()
    {
      close();
    }
    
    void Service::close()
    {
      if (listenSocket != -1) {
        ::shutdown(listenSocket,SHUT_RDWR);
        listenSocket = -1;
      }

      /* no 'disconnect' required here - this is a in_tra_ comm among
         the server ranks, not the externally facing connection */
      if (comm.comm != MPI_COMM_NULL) {
        MPI_CALL(Comm_free(&comm.comm));
        comm.setTo(MPI_COMM_NULL);
      }
    }
    
    /*! create a new service on given port (on host of rank 0), and
      return a handle t this service. A client can Service::connect()
      to this port, and we can service->accept() these connection
      requests */
    std::shared_ptr<Service> Service::create(MPI_Comm servingGroup, int port)
    {
      std::shared_ptr<Service> service = std::make_shared<Service>();
      service->comm = mpi::Group(servingGroup).dup();
      
      if (service->comm.rank == 0) {
        
        // socket at master - the one that's listening for new connections
        service->listenSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (service->listenSocket == INVALID_SOCKET)
          throw std::runtime_error("cannot create socket");
        
        /* When the server completes, the server socket enters a time-wait
           state during which the local address and port used by the
           socket are believed to be in use by the OS. The wait state may
           last several minutes. This socket option allows bind() to reuse
           the port immediately. */
#ifdef SO_REUSEADDR
        { int flag = true; ::setsockopt(service->listenSocket, SOL_SOCKET, SO_REUSEADDR,
                                        (const char*)&flag, sizeof(int)); }
#endif
        
        /*! bind socket to port */
        struct sockaddr_in serv_addr;
        memset((char *) &serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = (unsigned short) htons(port);
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        
        if (::bind(service->listenSocket, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
          throw std::runtime_error("binding to port failed");
        
        /*! listen to port, up to 5 pending connections */
        if (::listen(service->listenSocket,5) < 0)
          throw std::runtime_error("listening on socket failed");
        
        {
          char hostName[10000];
          gethostname(hostName,10000);
          
          std::stringstream ss;
          ss << hostName << ":" << ntohs(serv_addr.sin_port);
          service->serviceAddress = ss.str();
        } 
        std::cout << "#volumeServer: now listening for connections on "
                  << service->serviceAddress << std::endl;
      }        
      return service;
    }

    /*! return a 'address' string that the clients can connect
      to. typically, this is of the form "<hostname>:<portNo>", where
      hostname is the name of the machine that runs rank 0, and portNo
      is the TCP/IP socket that this service is published on */
    std::string Service::getServiceAddress() const
    {
      return serviceAddress;
    }
    
    /*! accept a new connection request. this will block until we
      either receive a timeout, or a client connection has been
      established */
    std::shared_ptr<Service::Connection> Service::accept()
    {
      char mpiPortName[MPI_MAX_PORT_NAME];
      MPI_Comm clientComm;
      
      MPI_CALL(Open_port(MPI_INFO_NULL,mpiPortName));
      if (comm.rank == 0) {
        
        /*! accept incoming connection */
        struct sockaddr_in addr;
        socklen_t sock_len = sizeof(addr);
        // std::cout << "#volumeServer: waiting for connection..." << std::endl;
        int acceptedSocket = ::accept(listenSocket, (struct sockaddr *) &addr, &sock_len);
        if (acceptedSocket == INVALID_SOCKET) 
          throw std::runtime_error("cannot accept connection");
        
        int len = strlen(mpiPortName);
        {
          ssize_t n = ::send(acceptedSocket,&len,sizeof(len),MSG_NOSIGNAL);
          if (n != sizeof(len))
            throw std::runtime_error("error sending data through port");
        }
        {
          ssize_t n = ::send(acceptedSocket,mpiPortName,len,MSG_NOSIGNAL);
          if (n != len)
            throw std::runtime_error("error sending data through port");
        }

        MPI_CALL(Barrier(comm.comm));
        MPI_CALL(Comm_accept(mpiPortName,MPI_INFO_NULL,0,
                             comm.comm,
                             &clientComm));
        ::shutdown(acceptedSocket,SHUT_RDWR);
      } else {
        MPI_CALL(Barrier(comm.comm));
        MPI_CALL(Comm_accept(NULL,MPI_INFO_NULL,0,
                             comm.comm,
                             &clientComm));
      }
      
      std::shared_ptr<Service::Connection> conn
        = std::make_shared<Service::Connection>();
      conn->setTo(clientComm);
      return  conn;
    }

    /*! have the provided mpi communicator create a connection to the
      service on 'serverAddr'. If that service hasn't been created (or
      for any other reason we cannot create a connection) we will
      throw an exception, else we return a handle to a connection that
      contains an intercomm to the MPI comm providing this service */
    std::shared_ptr<Service::Connection>
    Service::connect(MPI_Comm _parentComm, const std::string &serverAddr)
    {
      mpi::Group parentComm(_parentComm);
      std::shared_ptr<Service::Connection> conn
        = std::make_shared<Service::Connection>();
      int sockfd = -1;
      std::string mpiPortName = "";
      if (parentComm.rank == 0) {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == INVALID_SOCKET) 
          throw std::runtime_error("cannot create socket");

        char buffer[serverAddr.size()+1];
        strcpy(buffer,serverAddr.c_str());
        char *servName = strtok(buffer,":");
        if (!servName)
          throw std::runtime_error("could not parse server address '"+serverAddr+"' into hostname:port");
        char *colon = strtok(NULL,":");
        if (!colon)
          throw std::runtime_error("could not parse server address '"+serverAddr+"' into hostname:port");
        int servPort = atoi(colon);
        
        /*! perform DNS lookup */
        struct hostent* server = ::gethostbyname(servName);
        if (server == nullptr)
          throw std::runtime_error("server "+std::string(servName)+" not found");
          
        /*! perform connection */
        struct sockaddr_in serv_addr;
        memset((char*)&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = (unsigned short) htons(servPort);
        memcpy((char*)&serv_addr.sin_addr.s_addr, (char*)server->h_addr, server->h_length);

        if (::connect(sockfd,(struct sockaddr*) &serv_addr,sizeof(serv_addr)) < 0)
          throw std::runtime_error("connection to server socket failed");

        int mpiPortNameLen;
        {
          ssize_t n = ::recv(sockfd, &mpiPortNameLen, sizeof(mpiPortNameLen), 0);
          if (n != sizeof(mpiPortNameLen))
            throw std::runtime_error("could not receive port name length");
        }
        char mpiPortNameBuf[mpiPortNameLen+1];
        {
          ssize_t n = ::recv(sockfd, mpiPortNameBuf, mpiPortNameLen, 0);
          if (n != mpiPortNameLen)
            throw std::runtime_error("could not receive port name length");
          mpiPortNameBuf[mpiPortNameLen] = 0;
        }
        mpiPortName = mpiPortNameBuf;
      }
      
      MPI_Comm newComm = MPI_COMM_NULL;
      const char *portStr = (parentComm.rank==0)?mpiPortName.c_str():NULL;
      MPI_CALL(Comm_connect(portStr,
                            MPI_INFO_NULL,
                            0,
                            parentComm.comm,
                            &newComm));
      
      // connection established!
      conn->setTo(newComm);
      
      return conn;
    }
    
  } // ::mpi
} // ::ospray
