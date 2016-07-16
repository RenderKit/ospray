// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

#include <string>
#include <type_traits>

#include <ospray/ospray.h>
#include "ospcommon/vec.h"

namespace ospray {
  namespace cpp    {

    class ManagedObject
    {

    public:

      // string
      virtual void set(const std::string &name, const std::string &v) = 0;

      // int
      virtual void set(const std::string &name, int v) = 0;
      virtual void set(const std::string &name, int v1, int v2) = 0;
      virtual void set(const std::string &name, int v1, int v2, int v3) = 0;

      // float
      virtual void set(const std::string &name, float v) = 0;
      virtual void set(const std::string &name, float v1, float v2) = 0;
      virtual void set(const std::string &name, float v1, float v2, float v3) = 0;

      // double
      virtual void set(const std::string &name, double v) = 0;
      virtual void set(const std::string &name, double v1, double v2) = 0;
      virtual void set(const std::string &name, double v1, double v2, double v3)= 0;

      // ospcommon::vec2
      virtual void set(const std::string &name, const ospcommon::vec2i &v) = 0;
      virtual void set(const std::string &name, const ospcommon::vec2f &v) = 0;

      // ospcommon::vec3
      virtual void set(const std::string &name, const ospcommon::vec3i &v) = 0;
      virtual void set(const std::string &name, const ospcommon::vec3f &v) = 0;

      // ospcommon::vec4
      virtual void set(const std::string &name, const ospcommon::vec4f &v) = 0;

      // void*
      virtual void set(const std::string &name, void *v) = 0;

      // OSPObject*
      virtual void set(const std::string &name, OSPObject v) = 0;

      //! Commit to ospray
      virtual void commit() const = 0;

      //! Get the underlying generic OSPObject handle
      virtual OSPObject object() const = 0;

      virtual ~ManagedObject() {}
    };

    //! \todo auto-commit mode

    template <typename OSP_TYPE = OSPObject>
    class ManagedObject_T : public ManagedObject
    {
    public:

      ManagedObject_T(OSP_TYPE object = nullptr);
      virtual ~ManagedObject_T();

      void set(const std::string &name, const std::string &v) override;

      void set(const std::string &name, int v) override;
      void set(const std::string &name, int v1, int v2) override;
      void set(const std::string &name, int v1, int v2, int v3) override;

      void set(const std::string &name, float v) override;
      void set(const std::string &name, float v1, float v2) override;
      void set(const std::string &name, float v1, float v2, float v3) override;

      void set(const std::string &name, double v) override;
      void set(const std::string &name, double v1, double v2) override;
      void set(const std::string &name, double v1, double v2, double v3) override;

      void set(const std::string &name, const ospcommon::vec2i &v) override;
      void set(const std::string &name, const ospcommon::vec2f &v) override;

      void set(const std::string &name, const ospcommon::vec3i &v) override;
      void set(const std::string &name, const ospcommon::vec3f &v) override;

      void set(const std::string &name, const ospcommon::vec4f &v) override;

      void set(const std::string &name, void *v) override;

      void set(const std::string &name, OSPObject v) override;

      template <typename OTHER_OSP_TYPE>
      void set(const std::string &name, const ManagedObject_T<OTHER_OSP_TYPE> &v);

      void commit() const override;

      OSPObject object() const override;

      //! Get the underlying specific OSP* handle
      OSP_TYPE handle() const;

    protected:

      OSP_TYPE m_object;
    };

    // Inlined function definitions ///////////////////////////////////////////////

    template <typename OSP_TYPE>
    inline ManagedObject_T<OSP_TYPE>::ManagedObject_T(OSP_TYPE object) :
      m_object(object)
    {
      using OSPObject_T = typename std::remove_pointer<OSPObject>::type;
      using OtherOSP_T  = typename std::remove_pointer<OSP_TYPE>::type;
      static_assert(std::is_same<osp::ManagedObject, OSPObject_T>::value ||
                    std::is_base_of<osp::ManagedObject, OtherOSP_T>::value,
                    "ManagedObject_T<OSP_TYPE> can only be instantiated with "
                    "OSPObject (a.k.a. osp::ManagedObject*) or one of its"
                    "decendants (a.k.a. the OSP* family of types).");
    }

    template <typename OSP_TYPE>
    inline ManagedObject_T<OSP_TYPE>::~ManagedObject_T()
    {
    }

    template <typename OSP_TYPE>
    inline void ManagedObject_T<OSP_TYPE>::set(const std::string &name,
                                               const std::string &v)
    {
      ospSetString(m_object, name.c_str(), v.c_str());
    }

    template <typename OSP_TYPE>
    inline void ManagedObject_T<OSP_TYPE>::set(const std::string &name, int v)
    {
      ospSet1i(m_object, name.c_str(), v);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject_T<OSP_TYPE>::set(const std::string &name,
                                               int v1, int v2)
    {
      ospSet2i(m_object, name.c_str(), v1, v2);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject_T<OSP_TYPE>::set(const std::string &name,
                                               int v1, int v2, int v3)
    {
      ospSet3i(m_object, name.c_str(), v1, v2, v3);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject_T<OSP_TYPE>::set(const std::string &name, float v)
    {
      ospSet1f(m_object, name.c_str(), v);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject_T<OSP_TYPE>::set(const std::string &name,
                                               float v1, float v2)
    {
      ospSet2f(m_object, name.c_str(), v1, v2);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject_T<OSP_TYPE>::set(const std::string &name,
                                               float v1, float v2, float v3)
    {
      ospSet3f(m_object, name.c_str(), v1, v2, v3);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject_T<OSP_TYPE>::set(const std::string &name, double v)
    {
      ospSet1f(m_object, name.c_str(), v);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject_T<OSP_TYPE>::set(const std::string &name,
                                               double v1, double v2)
    {
      ospSet2f(m_object, name.c_str(), v1, v2);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject_T<OSP_TYPE>::set(const std::string &name,
                                               double v1, double v2, double v3)
    {
      ospSet3f(m_object, name.c_str(), v1, v2, v3);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject_T<OSP_TYPE>::set(const std::string &name,
                                               const ospcommon::vec2i &v)
    {
      ospSetVec2i(m_object, name.c_str(), (const osp::vec2i&)v);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject_T<OSP_TYPE>::set(const std::string &name,
                                               const ospcommon::vec2f &v)
    {
      ospSetVec2f(m_object, name.c_str(), (const osp::vec2f&)v);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject_T<OSP_TYPE>::set(const std::string &name,
                                               const ospcommon::vec3i &v)
    {
      ospSetVec3i(m_object, name.c_str(), (const osp::vec3i&)v);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject_T<OSP_TYPE>::set(const std::string &name,
                                               const ospcommon::vec3f &v)
    {
      ospSetVec3f(m_object, name.c_str(), (const osp::vec3f&)v);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject_T<OSP_TYPE>::set(const std::string &name,
                                               const ospcommon::vec4f &v)
    {
      ospSetVec4f(m_object, name.c_str(), (const osp::vec4f&)v);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject_T<OSP_TYPE>::set(const std::string &name, void *v)
    {
      ospSetVoidPtr(m_object, name.c_str(), v);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject_T<OSP_TYPE>::set(const std::string &name, OSPObject v)
    {
      ospSetObject(m_object, name.c_str(), v);
    }

    template <typename OSP_TYPE>
    template <typename OTHER_OSP_TYPE>
    inline void
    ManagedObject_T<OSP_TYPE>::set(const std::string &name,
                                   const ManagedObject_T<OTHER_OSP_TYPE> &v)
    {
      ospSetObject(m_object, name.c_str(), v.handle());
    }

    template <typename OSP_TYPE>
    inline void ManagedObject_T<OSP_TYPE>::commit() const
    {
      ospCommit(m_object);
    }

    template <typename OSP_TYPE>
    OSPObject ManagedObject_T<OSP_TYPE>::object() const
    {
      return (OSPObject)m_object;
    }

    template <typename OSP_TYPE>
    inline OSP_TYPE ManagedObject_T<OSP_TYPE>::handle() const
    {
      return m_object;
    }

  }// namespace cpp
}// namespace ospray
