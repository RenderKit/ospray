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

#include "ObjectHandle.h"
#include <map>
#include <stack>

namespace ospray {

  static std::map<int64,Ref<ospray::ManagedObject>> objectByHandle;
  static std::stack<int64> freedHandles;

  //! next unassigned ID on this node
  /*! we start numbering with 1 to make sure that "0:0" is an
    invalid handle (so we can typecast between (64-bit) handles
    and (64-bit)OSPWhatEver pointers */
  static int32 nextFreeLocalID = 1;

  void ObjectHandle::free()
  {
    freedHandles.push((int64)*this);
  }

  ObjectHandle::ObjectHandle()
  {
    if (freedHandles.empty()) {
      i32.ID    = nextFreeLocalID++;
      i32.owner = 0;
    } else {
      i64 = freedHandles.top();
      freedHandles.pop();
    }
  }

  ObjectHandle::ObjectHandle(int64 i) : i64(i)
  {
  }

  ObjectHandle::ObjectHandle(const ObjectHandle &other) : i64(other.i64)
  {
  }

  ObjectHandle &ObjectHandle::operator=(const ObjectHandle &other)
  {
    i64 = other.i64;
    return *this;
  }

  /*! define the given handle to refer to given object */
  void ObjectHandle::assign(const ObjectHandle &handle, ManagedObject *object)
  {
    objectByHandle[handle] = object;
  }

  void ObjectHandle::assign(ManagedObject *object) const
  {
    objectByHandle[*this] = object;
  }

  void ObjectHandle::freeObject() const
  {
    auto it = objectByHandle.find(i64);
    Assert(it != objectByHandle.end());
    it->second = nullptr;
    objectByHandle.erase(it);
  }

  int32 ObjectHandle::ownerRank() const
  {
    return i32.owner;
  }

  int32 ObjectHandle::objID() const
  {
    return i32.ID;
  }

  ospray::ObjectHandle::operator int64() const
  {
    return i64;
  }

  bool ObjectHandle::defined() const
  {
    auto it = objectByHandle.find(i64);
    return it != objectByHandle.end();
  }

  ManagedObject *ObjectHandle::lookup() const
  {
    if (i64 == 0) return nullptr;

    auto it = objectByHandle.find(i64);
    if (it == objectByHandle.end()) {
#ifndef NDEBUG
      // iw - made this into a warning only; the original code had
      // this throw an actual exceptoin, but that may be overkill
      std::cout << "#osp: WARNING: ospray is trying to look up object handle "+std::to_string(i64)+" that isn't defined!" << std::endl;
#endif
      return nullptr;
    }
    return it->second.ptr;
  }

  ObjectHandle ObjectHandle::lookup(ManagedObject *object)
  {
    for (auto it = objectByHandle.begin(); it != objectByHandle.end(); it++) {
      if (it->second.ptr == object) return(ObjectHandle(it->first));
    }

    return(nullHandle);
  }

  OSPRAY_SDK_INTERFACE const ObjectHandle nullHandle(0);

} // ::ospray
