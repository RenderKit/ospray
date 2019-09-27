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

// ospcommon
#include "ospcommon/utility/Any.h"
#include "ospcommon/utility/ParameterizedObject.h"
// ospray
#include "./OSPCommon.h"
// stl
#include <set>
#include <vector>

namespace ospray {

  struct Data;
  template <typename T, int DIM>
  struct DataT;

  struct OSPRAY_SDK_INTERFACE ManagedObject
      : public memory::RefCount,
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

    ManagedObject *getParamObject(const char *name,
                                  ManagedObject *valIfNotFound = nullptr);

    template <typename T, int DIM = 1>
    const DataT<T, DIM> *getParamDataT(const char *name, bool required = false);

    void checkUnused();

    virtual box3f getBounds() const;

    // Data members //

    void *ispcEquivalent{nullptr};
    OSPDataType managedObjectType{OSP_OBJECT};
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

  template <>
  inline Data *ManagedObject::getParam<Data *>(
      const char *name, Data *valIfNotFound)
  {
    auto *obj = ParameterizedObject::getParam<ManagedObject *>(
        name, (ManagedObject *)valIfNotFound);
    if (obj && obj->managedObjectType == OSP_DATA)
      return (Data *)obj;
    else
      return valIfNotFound;
  }

}  // namespace ospray

// Specializations for ISPCDevice /////////////////////////////////////////////

namespace ospcommon {
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

    template <>
    inline void ParameterizedObject::setParam<bool>(const std::string &name,
                                                    const bool &v)
    {
      setParam<int>(name, v);
    }

    template <>
    inline bool ParameterizedObject::getParam<bool>(const std::string &name,
                                                    bool valIfNotFound)
    {
      return getParam<int>(name, valIfNotFound);
    }

  }  // namespace utility
}  // namespace ospcommon
