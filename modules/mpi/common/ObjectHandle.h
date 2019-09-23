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

#pragma once

#include "common/Managed.h"
#include "common/OSPCommon.h"

namespace ospray {

struct ManagedObject;

#define NULL_HANDLE (ObjectHandle(0))

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
union OSPRAY_SDK_INTERFACE ObjectHandle
{
  void free();

  ObjectHandle();
  ObjectHandle(int64 i);
  ObjectHandle(const ObjectHandle &other);
  ObjectHandle &operator=(const ObjectHandle &other);

  /*! look up an object by handle, and return it. must be a defined handle */
  ManagedObject *lookup() const;

  /*! Return the handle associated with the given object. */
  static ObjectHandle lookup(ManagedObject *object);

  /*! check whether the handle is defined *on this rank* */
  bool defined() const;

  /*! define the given handle to refer to given object */
  static void assign(const ObjectHandle &handle, ManagedObject *object);

  /*! define the given handle to refer to given object */
  void assign(ManagedObject *object) const;

  void freeObject() const;

  int32 ownerRank() const;
  int32 objID() const;

  /*! cast to int64 to allow fast operations with this type */
  operator int64() const;

  // Data members //

  struct
  {
    int32 ID;
    int32 owner;
  } i32;

  int64 i64;
};

OSPRAY_SDK_INTERFACE extern const ObjectHandle nullHandle;

// Inlined operator definitions /////////////////////////////////////////////

inline bool operator==(const ObjectHandle &a, const ObjectHandle &b)
{
  return a.i64 == b.i64;
}

inline bool operator!=(const ObjectHandle &a, const ObjectHandle &b)
{
  return !(a == b);
}

} // namespace ospray

