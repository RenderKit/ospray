// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "rkcommon/utility/Any.h"
#include "rkcommon/utility/Optional.h"
#include "rkcommon/utility/ParameterizedObject.h"
// ospray
#include "./OSPCommon.h"
// stl
#include <set>
#include <vector>

namespace ospray {

struct Data;
template <typename T, int DIM>
struct DataT;

struct OSPRAY_CORE_INTERFACE ManagedObject : public memory::RefCount,
                                             public utility::ParameterizedObject
{
  using OSP_PTR = ManagedObject *;

  ManagedObject() = default;

  virtual ~ManagedObject() override;

  virtual void commit();

  virtual std::string toString() const;

  template <typename T>
  T getParam(const char *name, T valIfNotFound = T());

  template <typename T>
  T *getParamObject(const char *name, T *valIfNotFound = nullptr);

  template <typename T>
  inline utility::Optional<T> getOptParam(const char *name);

  void checkUnused();

  virtual box3f getBounds() const;

  // Data members //

  OSPDataType managedObjectType{OSP_OBJECT};

 private:
  template <typename T>
  inline bool checkObjType(T, OSPDataType);
};

OSPTYPEFOR_SPECIALIZATION(ManagedObject *, OSP_OBJECT);

// Inlined ManagedObject definitions ////////////////////////////////////////

template <typename T>
inline T ManagedObject::getParam(const char *name, T valIfNotFound)
{
  static_assert(!std::is_convertible<T, ManagedObject *>::value,
      "Use getParamObject<> for Obj parameters");
  return ParameterizedObject::getParam<T>(name, valIfNotFound);
}

template <typename T>
inline T *ManagedObject::getParamObject(const char *name, T *valIfNotFound)
{
  static_assert(std::is_convertible<T *, ManagedObject *>::value,
      "Can only get objects derived from ManagedObject");
  Param *const p = findParam(name);
  if (!p)
    return valIfNotFound;
  const bool prevQ = p->query;

  // objects are always set as ManagedObject*, retrieve as such
  auto *obj = ParameterizedObject::getParam<ManagedObject *>(
      name, (ManagedObject *)valIfNotFound);
  if (obj && obj->managedObjectType == OSPTypeFor<T *>::value)
    return (T *)obj;

  // last resort, e.g. for Texture2D
  if (dynamic_cast<T *>(obj))
    return (T *)obj;

  // reset query status if object is of incorrect type
  p->query = prevQ;
  return valIfNotFound;
}

template <typename T>
inline bool ManagedObject::checkObjType(T, OSPDataType)
{
  return true;
};

template <>
inline bool ManagedObject::checkObjType(ManagedObject *o, OSPDataType t)
{
  return o->managedObjectType == t;
};

template <typename T>
inline utility::Optional<T> ManagedObject::getOptParam(const char *name)
{
  utility::Optional<T> retval;
  Param *param = findParam(name);
  // objects are always set as ManagedObject*, retrieve as such
  using S =
      typename std::conditional<std::is_convertible<T, ManagedObject *>::value,
          ManagedObject *,
          T>::type;
  if (param && param->data.is<S>()) {
    auto val = param->data.get<S>();
    if (checkObjType(val, OSPTypeFor<T>::value)) {
      retval = (T)val;
      param->query = true;
    }
  }
  return retval;
}

} // namespace ospray

// Specializations for ISPCDevice /////////////////////////////////////////////

namespace rkcommon {
namespace utility {

template <>
inline void ParameterizedObject::Param::set(
    const ospray::ManagedObject::OSP_PTR &object)
{
  using OSP_PTR = ospray::ManagedObject::OSP_PTR;

  if (object)
    object->refInc();

  if (data.is<OSP_PTR>()) {
    auto *existingObj = data.get<OSP_PTR>();
    if (existingObj != nullptr)
      existingObj->refDec();
  }

  data = object;
}

} // namespace utility
} // namespace rkcommon
