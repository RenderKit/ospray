// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// stl
#include <string>
#include <type_traits>
// ospray
#include "ospray/ospray_util.h"
// ospcommon
#include "ospcommon/math/AffineSpace.h"
#include "ospcommon/math/vec.h"
// ospray::cpp
#include "Traits.h"

namespace ospray {
namespace cpp {

using namespace ospcommon::math;

template <typename HANDLE_T = OSPObject, OSPDataType TYPE = OSP_OBJECT>
class ManagedObject
{
 public:
  static constexpr OSPDataType type = TYPE;

  ManagedObject(HANDLE_T object = nullptr);
  ~ManagedObject();

  ManagedObject(const ManagedObject<HANDLE_T, TYPE> &copy);
  ManagedObject(ManagedObject<HANDLE_T, TYPE> &&move);

  ManagedObject &operator=(const ManagedObject<HANDLE_T, TYPE> &copy);
  ManagedObject &operator=(ManagedObject<HANDLE_T, TYPE> &&move);

  template <typename T>
  void setParam(const std::string &name, const T &v) const;

  void setParam(const std::string &name, const std::string &v) const;

  void setParam(const std::string &name, OSPDataType, const void *) const;

  void removeParam(const std::string &name) const;

  box3f getBounds() const;

  void commit() const;

  //! Get the underlying specific OSP* handle
  HANDLE_T handle() const;

  //! return whether the given object is valid, or NULL
  operator bool() const;

 protected:
  HANDLE_T ospObject;
};

// Inlined function definitions ///////////////////////////////////////////

template <typename HANDLE_T, OSPDataType TYPE>
inline ManagedObject<HANDLE_T, TYPE>::ManagedObject(HANDLE_T object)
    : ospObject(object)
{
  using OSPObject_T = typename std::remove_pointer<OSPObject>::type;
  using OtherOSP_T = typename std::remove_pointer<HANDLE_T>::type;
  static_assert(std::is_same<osp::ManagedObject, OSPObject_T>::value
          || std::is_base_of<osp::ManagedObject, OtherOSP_T>::value,
      "ManagedObject<HANDLE_T> can only be instantiated with "
      "OSPObject (a.k.a. osp::ManagedObject*) or one of its"
      "descendants (a.k.a. the OSP* family of types).");
}

template <typename HANDLE_T, OSPDataType TYPE>
inline ManagedObject<HANDLE_T, TYPE>::~ManagedObject()
{
  if (ospObject)
    ospRelease(ospObject);
}

template <typename HANDLE_T, OSPDataType TYPE>
inline ManagedObject<HANDLE_T, TYPE>::ManagedObject(
    const ManagedObject<HANDLE_T, TYPE> &copy)
{
  ospObject = copy.handle();
  ospRetain(copy.handle());
}

template <typename HANDLE_T, OSPDataType TYPE>
inline ManagedObject<HANDLE_T, TYPE>::ManagedObject(
    ManagedObject<HANDLE_T, TYPE> &&move)
{
  ospObject = move.handle();
  move.ospObject = nullptr;
}

template <typename HANDLE_T, OSPDataType TYPE>
inline ManagedObject<HANDLE_T, TYPE> &ManagedObject<HANDLE_T, TYPE>::operator=(
    const ManagedObject<HANDLE_T, TYPE> &copy)
{
  ospObject = copy.handle();
  ospRetain(copy.handle());
  return *this;
}

template <typename HANDLE_T, OSPDataType TYPE>
inline ManagedObject<HANDLE_T, TYPE> &ManagedObject<HANDLE_T, TYPE>::operator=(
    ManagedObject<HANDLE_T, TYPE> &&move)
{
  ospObject = move.handle();
  move.ospObject = nullptr;
  return *this;
}

template <typename HANDLE_T, OSPDataType TYPE>
template <typename T>
inline void ManagedObject<HANDLE_T, TYPE>::setParam(
    const std::string &name, const T &v) const
{
  static_assert(OSPTypeFor<T>::value != OSP_UNKNOWN,
      "Only types corresponding to OSPDataType values can be set "
      "as parameters on OSPRay objects. NOTE: Math types (vec, "
      "box, linear, affine) are "
      "expected to come from ospcommon::math.");
  ospSetParam(ospObject, name.c_str(), OSPTypeFor<T>::value, &v);
}

template <typename HANDLE_T, OSPDataType TYPE>
inline void ManagedObject<HANDLE_T, TYPE>::setParam(
    const std::string &name, const std::string &v) const
{
  ospSetParam(ospObject, name.c_str(), OSP_STRING, v.c_str());
}

template <typename HANDLE_T, OSPDataType TYPE>
void ManagedObject<HANDLE_T, TYPE>::setParam(
    const std::string &name, OSPDataType type, const void *mem) const
{
  ospSetParam(ospObject, name.c_str(), type, mem);
}

template <typename HANDLE_T, OSPDataType TYPE>
inline void ManagedObject<HANDLE_T, TYPE>::removeParam(
    const std::string &name) const
{
  ospRemoveParam(ospObject, name.c_str());
}

template <typename HANDLE_T, OSPDataType TYPE>
box3f ManagedObject<HANDLE_T, TYPE>::getBounds() const
{
  auto b = ospGetBounds(ospObject);
  return box3f(vec3f(b.lower), vec3f(b.upper));
}

template <typename HANDLE_T, OSPDataType TYPE>
inline void ManagedObject<HANDLE_T, TYPE>::commit() const
{
  ospCommit(ospObject);
}

template <typename HANDLE_T, OSPDataType TYPE>
inline HANDLE_T ManagedObject<HANDLE_T, TYPE>::handle() const
{
  return ospObject;
}

template <typename HANDLE_T, OSPDataType TYPE>
inline ManagedObject<HANDLE_T, TYPE>::operator bool() const
{
  return handle() != nullptr;
}

} // namespace cpp
} // namespace ospray
