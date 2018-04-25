// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

#include <cstdlib>
#include <cstdint>
#include <string>
#include <vector>
#include "Socket.h"
#include "Fabric.h"

namespace ospcommon {
  namespace networking {

    /*! A fabrich which sends and recieves over a TCP socket connection */
    struct OSPCOMMON_INTERFACE SocketFabric : public Fabric
    {
      /*! Setup the connection by listening for an incoming
          connection on the desired port */
      SocketFabric(const uint16_t port);
      /*! Connecting to another socket on the host on the desired port */
      SocketFabric(const std::string &hostname, const uint16_t port);
      ~SocketFabric() override;

      SocketFabric(const SocketFabric&) = delete;
      SocketFabric& operator=(const SocketFabric&) = delete;

      /*! send exact number of bytes - the fabric can do that through
        multiple smaller messages, but all bytes have to be
        delivered */
      void send(void *mem, size_t s) override;

      /*! receive some block of data - whatever the sender has sent -
        and give us size and pointer to this data */
      size_t read(void *&mem) override;

    private:
      ospcommon::socket_t socket;
      std::vector<char> buffer;
    };

  } // ::ospcommon::networking
} // ::ospcommon

