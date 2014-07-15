/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

/*! \todo move to api - it's also used in coi, and not mpi specific */

#include "ospray/common/ospcommon.h"
#include "ospray/common/managed.h"

namespace ospray {
  namespace api {

#define NULL_HANDLE (Handle(0))

    //! (local) handle to a (remote) managed object
    /*! abstraction for a remotely-held 'managed object'. the handle
      refers to both 'owner' (the machine that has it) as well as to a
      local ID (by which that owner can look it up). Note that other
      ranks may also have copies of that object. 

      note that the 'null handle' is '0', not -1. This allows an app
      to test the handled resturend from ospNewXXX calls for null just
      as if they were pointers (and thus, 'null' objects are
      consistent between local and mpi rendering)
    */
    struct Handle {
      static Handle alloc();
      void free();

      inline Handle(const int64 i) : i64(i) {};
      inline Handle(const Handle &other=NULL_HANDLE) : i64(other.i64) {};
      inline Handle &operator=(const Handle &other) { i64=other.i64; return *this; }

      union {
        struct { int32 ID; int32 owner; } i32;
        int64 i64;
      };

      /*! look up an object by handle, and return it. must be a defiend handle */
      ManagedObject *lookup() const;
      /*! check whether the handle is defined *on this rank* */
      bool defined() const;
      /*! define the given handle to refer to given object */
      static void assign(const Handle &handle, const ManagedObject *object);
      /*! define the given handle to refer to given object */
      void assign(const ManagedObject *object) const;
      //! free the given object
      void freeObject() const;
      /*! returns the owner's rank */
      inline int32 owner() const { return i32.owner; }
      /*! returns the local ID to reference this object on the owner rank */
      inline int32 ID() const { return i32.ID; }
      /*! cast to int64 to allow fast operations with this type */
      inline operator int64() const { return i64; }

      static const Handle nullHandle;
    };

    inline bool operator==(const Handle &a, const Handle &b) { return a.i64==b.i64; }
    inline bool operator!=(const Handle &a, const Handle &b) { return a.i64!=b.i64; }
  }
}
