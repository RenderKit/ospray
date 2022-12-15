// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "MPICommon.h"
#include "common/OSPCommon.h"
#include "ospray_mpi_common_export.h"
#include "rkcommon/memory/RefCount.h"

namespace ospray {

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
union OSPRAY_MPI_COMMON_EXPORT ObjectHandle
{
  ObjectHandle();
  ObjectHandle(int64 i);
  ObjectHandle(const ObjectHandle &other);
  ObjectHandle &operator=(const ObjectHandle &other);

  void free();

  /*! look up an object by handle, and return it. must be a defined handle */
  memory::RefCount *lookup() const;

  /* Allocate a local ObjectHandle */
  static ObjectHandle allocateLocalHandle();

  /*! Return the handle associated with the given object. */
  static ObjectHandle lookup(memory::RefCount *object);

  /*! check whether the handle is defined *on this rank* */
  bool defined() const;

  /*! define the given handle to refer to given object */
  static void assign(const ObjectHandle &handle, memory::RefCount *object);

  /*! define the given handle to refer to given object */
  void assign(memory::RefCount *object) const;

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

OSPRAY_MPI_COMMON_EXPORT extern const ObjectHandle nullHandle;

// Inlined operator definitions /////////////////////////////////////////////

OSPRAY_MPI_COMMON_EXPORT inline bool operator==(
    const ObjectHandle &a, const ObjectHandle &b)
{
  return a.i64 == b.i64;
}

OSPRAY_MPI_COMMON_EXPORT inline bool operator!=(
    const ObjectHandle &a, const ObjectHandle &b)
{
  return !(a == b);
}

template <typename OSPRAY_TYPE>
inline OSPRAY_TYPE *lookupObject(OSPObject obj)
{
  auto &handle = reinterpret_cast<ObjectHandle &>(obj);
  return handle.defined() ? (OSPRAY_TYPE *)handle.lookup() : (OSPRAY_TYPE *)obj;
}

} // namespace ospray
