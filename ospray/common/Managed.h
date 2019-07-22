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
#include "common/OSPCommon.h"
#include "common/ObjectHandle.h"
#include "ospray/OSPDataType.h"
// stl
#include <set>
#include <vector>

namespace ospray {

  struct Data;

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

    Data *getParamData(const char *name, Data *valIfNotFound = nullptr);

    vec4f getParam4f(const char *name, vec4f valIfNotFound);
    vec3fa getParam3f(const char *name, vec3fa valIfNotFound);
    vec3f getParam3f(const char *name, vec3f valIfNotFound);
    vec3i getParam3i(const char *name, vec3i valIfNotFound);
    vec2f getParam2f(const char *name, vec2f valIfNotFound);
    int32 getParam1i(const char *name, int32 valIfNotFound);
    float getParam1f(const char *name, float valIfNotFound);
    bool getParam1b(const char *name, bool valIfNotFound);

    affine3f getParamAffine3f(const char *name, affine3f valIfNotFound);

    void *getParamVoidPtr(const char *name, void *valIfNotFound);
    std::string getParamString(const char *name,
                               std::string valIfNotFound = "");

    // Data members //

    void *ispcEquivalent{nullptr};
    OSPDataType managedObjectType{OSP_UNKNOWN};
  };

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

  inline Data *ManagedObject::getParamData(const char *name,
                                           Data *valIfNotFound)
  {
    return (Data *)getParamObject(name, (ManagedObject *)valIfNotFound);
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
