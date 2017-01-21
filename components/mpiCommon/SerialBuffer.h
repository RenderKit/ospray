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

#pragma once

#include <vector>
#include <type_traits>
#include <unordered_map>

#include "mpiCommon/MPICommon.h"
#include "mpiCommon/command.h"

namespace ospray {
  namespace mpi {
    namespace work {

      struct OSPRAY_MPI_INTERFACE SerialBuffer
      {
        std::vector<byte_t> buffer;
        size_t index;
        // size_t bytesAvailable;

        SerialBuffer(size_t sz = 1024 * 32);
        // get data at current index  WARNING: if realloced, ptrs invalidated
        byte_t* getPtr();
        // get data at index  WARNING: if realloced, ptrs invalidated
        byte_t* getPtr(size_t ind) { return &buffer.at(ind); }
        // return data at index 0 WARNING: if realloced, ptrs invalidated
        byte_t* getData() { return buffer.data(); }
        void write(byte_t* data, size_t size);
        void read(byte_t* data, size_t size);
        //reserve size elements from current index
        void reserve(size_t size);
        void clear();  //resets index to 0, does not actually clear out memory
        size_t getIndex() { return index; }
        void setIndex(size_t i) { index = i; }
        // size_t size() const;
        // bool empty() const;
        //Carson: empty and size depend a bit on how you are utlizing the
        //        buffer not sure how the bytesRemaining was meant to be
        //        used - it's a self allocating buffer
      };

      struct OSPRAY_MPI_INTERFACE Work
      {
        friend SerialBuffer& operator<<(SerialBuffer& b, const Work &work);
        friend SerialBuffer& operator>>(SerialBuffer& b, Work &work);

        virtual void run() {}
        // Run the master-side variant of this command in master/worker mode
        // The default just does nothing
        virtual void runOnMaster() {}
        virtual size_t getTag() const = 0;
        // Check whether this work unit should flush the command buffer,
        // the default returns false
        virtual bool flushing() const { return false; }

        // Have the child work unit write its data to the buffer
        virtual void serialize(SerialBuffer &b) const = 0;
        // Have the child work unit read its data from the buffer
        virtual void deserialize(SerialBuffer &b) = 0;

        using WorkMap = std::unordered_map<size_t, Work* (*)()>;
        // NOTE(jda) - No longer const as this is now populated on mpi module
        //             load
        static WorkMap WORK_MAP;
      };

      // NOTE(jda) - I'd rather use constexpr here to give a name to value in
      //             the enable_if, but alas that's only supported in VS2015+...
      template <typename T>
      using enable_SerialBuffer_operator_t =
        typename std::enable_if<!std::is_pointer<T>::value &&
                                !std::is_same<T, Work>::value &&
                                !std::is_same<T, std::string>::value,
                                SerialBuffer&>::type;

      // We can't take types which aren't POD or which are pointers
      // because there's no sensible way to serialize/deserialize them.
      // Unfortunately while the vecNT types are definitely just trivially
      // copy-able data they don't meet the template constraints to be pod or
      // trivially_copyable so we can't use that as a filter test
      template<typename T>
      enable_SerialBuffer_operator_t<T>
      inline operator<<(SerialBuffer &buf, const T &rh)
      {
        buf.write((byte_t*)&rh, sizeof(T));
        return buf;
      }

      template<typename T>
      enable_SerialBuffer_operator_t<T>
      inline operator>>(SerialBuffer &buf, T &rh)
      {
        buf.read((byte_t*)&rh, sizeof(T));
        return buf;
      }

      template<typename T>
      typename std::enable_if<!std::is_pointer<T>::value, SerialBuffer&>::type
      inline operator<<(SerialBuffer &buf, const std::vector<T> &rh)
      {
        buf << rh.size();
        buf.write((byte_t*)rh.data(), rh.size() * sizeof(T));
        return buf;
      }

      template<typename T>
      typename std::enable_if<!std::is_pointer<T>::value, SerialBuffer&>::type
      inline operator>>(SerialBuffer &buf, std::vector<T> &rh)
      {
        size_t size;
        buf >> size;
        rh = std::vector<T>(size, T());
        buf.read((byte_t*)rh.data(), rh.size() * sizeof(T));
        return buf;
      }

      // Child work classes should can implement these but should probably
      // just go through serialize/deserialize so we can generically serialize
      // work units into a buffer.
      inline SerialBuffer& operator<<(SerialBuffer &b, const Work &work)
      {
        work.serialize(b);
        return b;
      }

      inline SerialBuffer& operator>>(SerialBuffer &b, Work &work)
      {
        work.deserialize(b);
        return b;
      }

      inline SerialBuffer &operator<<(SerialBuffer &buf, const std::string &rh)
      {
        buf << rh.size();
        buf.write((byte_t*)rh.c_str(), rh.size());
        return buf;
      }

      inline SerialBuffer &operator>>(SerialBuffer &buf, std::string &rh)
      {
        size_t size;
        buf >> size;
        rh = std::string(size, ' ');
        buf.read((byte_t*)&rh[0], size);
        return buf;
      }

      // Decode the commands in the buffer, appending them to the cmds vector.
      // TODO: Should we be using std::shared_ptr here instead of raw pointers?
      OSPRAY_MPI_INTERFACE void decode_buffer(SerialBuffer &buf,
                                              std::vector<Work*> &cmds,
                                              const int numMessages);
      OSPRAY_MPI_INTERFACE void debug_log_messages(SerialBuffer &buf,
                                                   const int numMessages);

      template<typename T>
      inline Work* make_work_unit()
      {
        return new T();
      }

    }// namespace work
  }// namespace mpi
}// namespace ospray
