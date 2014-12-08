/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "Handle.h"
#include <map>
#include <stack>

namespace ospray {

  namespace api {
    std::map<int64,Ref<ospray::ManagedObject> > objectByHandle;
    std::stack<int64> freedHandles;

    //! next unassigned ID on this node
    /*! we start numbering with 1 to make sure that "0:0" is an
        invalid handle (so we can typecast between (64-bit) handles
        and (64-bit)OSPWhatEver pointers */
    int32 nextFreeLocalID = 1;
    
    void Handle::free()
    {
      freedHandles.push((int64)*this);
    }
    Handle Handle::alloc()
    {
      Handle h;
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
    void Handle::assign(const Handle &handle, const ManagedObject *object)
    {
      objectByHandle[handle] = (ManagedObject*)object;
    }
    void Handle::assign(const ManagedObject *object) const
    {
      objectByHandle[*this] = (ManagedObject*)object;
    }
    void Handle::freeObject() const
    {
      std::map<int64,Ref<ManagedObject> >::iterator it = 
        objectByHandle.find(i64);
      Assert(it != objectByHandle.end());
      it->second = NULL;
      objectByHandle.erase(it);
    }
    bool Handle::defined() const 
    {
      std::map<int64,Ref<ManagedObject> >::const_iterator it = 
        objectByHandle.find(i64);
      return it != objectByHandle.end();
    }
    ManagedObject *Handle::lookup() const
    {
      if (i64 == 0) return NULL;

      std::map<int64,Ref<ManagedObject> >::const_iterator it = 
        objectByHandle.find(i64);
      Assert(it != objectByHandle.end());
      return it->second.ptr;
    }
    
    const Handle Handle::nullHandle(0);
  }
}
