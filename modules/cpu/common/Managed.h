// Copyright 2009-2020 Intel Corporation
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

struct OSPRAY_SDK_INTERFACE ManagedObject : public memory::RefCount,
                                            public utility::ParameterizedObject
{
  using OSP_PTR = ManagedObject *;

  ManagedObject() = default;

  virtual ~ManagedObject() override;

  virtual void commit();

  virtual std::string toString() const;

  void *getIE() const;

  template <typename T>
  T getParam(const char *name, T valIfNotFound = T());

  template <typename T>
  inline utility::Optional<T> getOptParam(const char *name);

  ManagedObject *getParamObject(
      const char *name, ManagedObject *valIfNotFound = nullptr);

  template <typename T, int DIM = 1>
  const Ref<const DataT<T, DIM>> getParamDataT(
      const char *name, bool required = false, bool promoteScalar = false);

  void checkUnused();

  virtual box3f getBounds() const;

  // Data members //

  void *ispcEquivalent{nullptr};
  OSPDataType managedObjectType{OSP_OBJECT};

 private:
  template <typename T>
  inline bool checkObjType(T, OSPDataType);
};

OSPTYPEFOR_SPECIALIZATION(ManagedObject *, OSP_OBJECT);

// Inlined ManagedObject definitions ////////////////////////////////////////

inline void *ManagedObject::getIE() const
{
  return ispcEquivalent;
}

template <typename T>
inline T ManagedObject::getParam(const char *name, T valIfNotFound)
{
  return ParameterizedObject::getParam<T>(name, valIfNotFound);
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

template <>
inline Data *ManagedObject::getParam<Data *>(
    const char *name, Data *valIfNotFound)
{
  auto *obj = ParameterizedObject::getParam<ManagedObject *>(
      name, (ManagedObject *)valIfNotFound);
  if (obj && obj->managedObjectType == OSP_DATA)
    return (Data *)obj;
  else {
    // reset query status if object is not a Data*
    if (obj)
      findParam(name)->query = false;
    return valIfNotFound;
  }
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
