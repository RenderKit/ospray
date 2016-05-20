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

#include "ObjectHandle.h"
#include <map>
#include <stack>

namespace ospray {

  std::map<int64,Ref<ospray::ManagedObject> > objectByHandle;
  std::stack<int64> freedHandles;

  //! next unassigned ID on this node
  /*! we start numbering with 1 to make sure that "0:0" is an
    invalid handle (so we can typecast between (64-bit) handles
    and (64-bit)OSPWhatEver pointers */
  int32 nextFreeLocalID = 1;
    
  void ObjectHandle::free()
  {
    freedHandles.push((int64)*this);
  }
  ObjectHandle ObjectHandle::alloc()
  {
    ObjectHandle h;
    if (freedHandles.empty()) {
      //      h.i32.owner = app.rank;
      h.i32.ID = nextFreeLocalID++;
    } else {
      h = freedHandles.top();
      freedHandles.pop();
    }
    return h;
  }

  /*! define the given handle to refer to given object */
  void ObjectHandle::assign(const ObjectHandle &handle,
                            const ManagedObject *object)
  {
    objectByHandle[handle] = (ManagedObject*)object;
  }

  void ObjectHandle::assign(const ManagedObject *object) const
  {
    objectByHandle[*this] = (ManagedObject*)object;
  }

  void ObjectHandle::freeObject() const
  {
    auto it = objectByHandle.find(i64);
    Assert(it != objectByHandle.end());
    it->second = NULL;
    objectByHandle.erase(it);
  }

  bool ObjectHandle::defined() const 
  {
    auto it = objectByHandle.find(i64);
    return it != objectByHandle.end();
  }

  ManagedObject *ObjectHandle::lookup() const
  {
    if (i64 == 0) return NULL;

    auto it = objectByHandle.find(i64);
    Assert(it != objectByHandle.end());
    return it->second.ptr;
  }

  ObjectHandle ObjectHandle::lookup(ManagedObject *object)
  {
    for (auto it = objectByHandle.begin(); it != objectByHandle.end(); it++) {
      if (it->second.ptr == object) return(ObjectHandle(it->first));
    }

    return(nullHandle);
  }
    
  const ObjectHandle ObjectHandle::nullHandle(0);

} // ::ospray
