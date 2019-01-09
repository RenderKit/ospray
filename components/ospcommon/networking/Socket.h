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

/*! iw: note this code is literally copied (and then adapted) from
    embree's original common/sys/network.h */

#pragma once

#include "../common.h"

namespace ospcommon
{
  /*! type for a socket */
  using socket_t = struct opaque_socket_t*;

  /*! exception thrown when other side disconnects */
  struct Disconnect : public std::exception
  {
#ifdef _WIN32
    virtual const char* what() const override
#else
    virtual const char* what() const noexcept override
#endif
    { return "network disconnect"; }
  };

  /*! creates a socket bound to a port */
  OSPCOMMON_INTERFACE socket_t bind(unsigned short port);

  /*! listens for an incoming connection and accepts that connection */
  OSPCOMMON_INTERFACE socket_t listen(socket_t sockfd);

  /*! initiates a connection */
  OSPCOMMON_INTERFACE socket_t connect(const char* host, unsigned short port);

  /*! read data from the socket */
  OSPCOMMON_INTERFACE void read(socket_t socket, void* data, size_t bytes);

  /*! read the available data on the socket, up to 'bytes' bytes.
      Returns the number of bytes read. */
  OSPCOMMON_INTERFACE size_t read_some(socket_t socket, void* data,
                                       const size_t bytes);

  /*! write data to the socket */
  OSPCOMMON_INTERFACE void write(socket_t socket, const void* data, size_t bytes);

  /*! flushes the write buffer */
  OSPCOMMON_INTERFACE void flush(socket_t socket);

  /*! close a socket */
  OSPCOMMON_INTERFACE void close(socket_t socket);

  /*! reads a bool from the socket */
  OSPCOMMON_INTERFACE bool read_bool(socket_t socket);

  /*! reads a character from the socket */
  OSPCOMMON_INTERFACE char read_char(socket_t socket);

  /*! reads an integer from the socket */
  OSPCOMMON_INTERFACE int read_int(socket_t socket);

  /*! reads a float from the socket */
  OSPCOMMON_INTERFACE float read_float(socket_t socket);

  /*! reads a string from the socket */
  OSPCOMMON_INTERFACE std::string read_string(socket_t socket);

  /*! writes a bool to the socket */
  OSPCOMMON_INTERFACE void write(socket_t socket, bool value);

  /*! writes a character to the socket */
  OSPCOMMON_INTERFACE void write(socket_t socket, char value);

  /*! writes an integer to the socket */
  OSPCOMMON_INTERFACE void write(socket_t socket, int value);

  /*! writes a float to the socket */
  OSPCOMMON_INTERFACE void write(socket_t socket, float value);

  /*! writes a string to the socket */
  OSPCOMMON_INTERFACE void write(socket_t socket, const std::string& str);

}// ::ospcommon
