#include "handle.h"
#include <map>

namespace ospray {

  namespace mpi {
    std::map<int64,Ref<ospray::ManagedObject> > objectByHandle;
    
    //! next unassigned ID on this node
    /*! we start numbering with 1 to make sure that "0:0" is an
        invalid handle (so we can typecast between (64-bit) handles
        and (64-bit)OSPWhatEver pointers */
    int32 nextFreeLocalID = 1;
    
    Handle Handle::alloc()
    {
      Handle h;
      h.i32.owner = app.rank;
      h.i32.ID = nextFreeLocalID++;
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
      std::map<int64,Ref<ManagedObject> >::const_iterator it = 
        objectByHandle.find(i64);
      Assert(it != objectByHandle.end());
      return it->second.ptr;
    }
  }
}
