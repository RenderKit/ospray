// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

#include "SerialBuffer.h"

namespace ospray {
  namespace mpi {
    namespace work {

      Work::WorkMap Work::WORK_MAP;

      SerialBuffer::SerialBuffer(size_t sz) : buffer(sz, 255), index(0)
      {
      }

      byte_t* SerialBuffer::getPtr()
      {
        return &buffer[index];
      }

      void SerialBuffer::write(byte_t* data, size_t size)
      {
        // TODO: This should not call reserve, it should only grow if we
        // actually need more space. Also it should use a different growing
        // strategy than just allocating 1k extra than requested
        reserve(size);
        memcpy(getPtr(), data, size);
        index += size;
        // bytesAvailable += size;
      }

      void SerialBuffer::read(byte_t* data, size_t size)
      {
        if (buffer.size() - getIndex() < size) {
          PRINT(buffer.size());
          PRINT(getIndex());
          throw std::runtime_error("SerialBuffer is out of room for read of"
                                   "size " + std::to_string(size));
        }
        memcpy(data, getPtr(), size);
        index += size;
        // bytesAvailable -= size;
      }

      void SerialBuffer::clear()
      {
        index = 0;
        // bytesAvailable = 0;
      }

      void SerialBuffer::reserve(size_t size)
      {
        if ((buffer.size() - getIndex()) < size)
        {
          /*
          std::cout << "warning: reallocating SerialBuffer, had "
            << buffer.size() << " bytes but need " << index + size << "\n";
            */
          // allocates over requested so we don't get a lot of small reallocations
          // TODO: This should not allocate more than requested
          buffer.resize(getIndex() + size + 1024);
          // bytesAvailable += size + 1024;
        }
      }

      void decode_buffer(SerialBuffer &buf,
                         std::vector<Work*> &cmds,
                         const int numMessages)
      {
        for (size_t i = 0; i < numMessages; ++i) {
          size_t type = 0;
          buf >> type;
          Work::WorkMap::const_iterator fnd = Work::WORK_MAP.find(type);
          if (fnd != Work::WORK_MAP.end()) {
            Work *w = (*fnd->second)();
            buf >> *w;
            cmds.push_back(w);
          } else {
            throw std::runtime_error("Unknown work message tag: " +
                                     std::to_string(type));
          }
        }
      }

      void debug_log_messages(SerialBuffer &buf, const int numMessages)
      {
        size_t startIndex = buf.getIndex();
        for (size_t i = 0; i < numMessages; ++i) {
          size_t type = 0;
          buf >> type;
          std::cout << "DBG Message[" << i << "] = "
                    << commandToString(CommandTag(type)) << "\n";
          Work::WorkMap::const_iterator fnd = Work::WORK_MAP.find(type);
          if (fnd != Work::WORK_MAP.end()) {
            Work *w = (*fnd->second)();
            buf >> *w;
            delete w;
          } else {
            throw std::runtime_error("Unknown work message tag: " +
                                     std::to_string(type));
          }
        }
        buf.setIndex(startIndex);
      }

    }// namespace work
  }// namespace mpi
}// namespace ospray
