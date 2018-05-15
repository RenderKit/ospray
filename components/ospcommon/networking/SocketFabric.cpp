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

#include "SocketFabric.h"

namespace ospcommon {
  namespace networking {

      SocketFabric::SocketFabric(const uint16_t port)
        : socket(nullptr), buffer(64 * 1024, 0)
      {
        ospcommon::socket_t listenSock = ospcommon::bind(port);
        socket = ospcommon::listen(listenSock);
      }

      SocketFabric::SocketFabric(const std::string &hostname, const uint16_t port)
        : socket(ospcommon::connect(hostname.c_str(), port)),
        buffer(64 * 1024, 0)
      {}

      SocketFabric::~SocketFabric() {
        ospcommon::close(socket);
      }

      void SocketFabric::send(void *mem, size_t s) {
        // A bit annoying, because the ospcommon::Socket wrapper does its
        // own internal buffering, however a Fabric is unbuffered and is
        // made buffered by using the buffered data streams
        ospcommon::write(socket, mem, s);
        ospcommon::flush(socket);
      }

      size_t SocketFabric::read(void *&mem) {
        const size_t s = ospcommon::read_some(socket, buffer.data(),
                                              buffer.size());
        mem = buffer.data();
        return s;
      }
  }
}

