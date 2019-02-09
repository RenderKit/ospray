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

#include <string>
#include <type_traits>

#include <ospray/ospray.h>
#include "ospcommon/vec.h"
#include "ospcommon/AffineSpace.h"

namespace ospray {
  namespace cpp    {

    class ManagedObject
    {

    public:

      // string
      virtual void set(const std::string &name, const std::string &v) const = 0;

      // int
      virtual void set(const std::string &name, int v) const = 0;
      virtual void set(const std::string &name, int v1, int v2) const = 0;
      virtual void set(const std::string &name, int v1, int v2, int v3) const = 0;

      // float
      virtual void set(const std::string &name, float v) const = 0;
      virtual void set(const std::string &name, float v1, float v2) const = 0;
      virtual void set(const std::string &name, float v1, float v2, float v3) const = 0;
      virtual void set(const std::string &name, float v1, float v2, float v3, float v4) const = 0;

      // double
      virtual void set(const std::string &name, double v) const = 0;
      virtual void set(const std::string &name, double v1, double v2) const = 0;
      virtual void set(const std::string &name, double v1, double v2, double v3) const = 0;
      virtual void set(const std::string &name, double v1, double v2, double v3, double v4) const = 0;

      // ospcommon::vec2
      virtual void set(const std::string &name, const ospcommon::vec2i &v) const = 0;
      virtual void set(const std::string &name, const ospcommon::vec2f &v) const = 0;

      // ospcommon::vec3
      virtual void set(const std::string &name, const ospcommon::vec3i &v) const = 0;
      virtual void set(const std::string &name, const ospcommon::vec3f &v) const = 0;

      // ospcommon::vec4
      virtual void set(const std::string &name, const ospcommon::vec4f &v) const = 0;

      // C-string
      virtual void set(const std::string &name, const char *v) const = 0;

      // void*
      virtual void set(const std::string &name, void *v) const = 0;

      // OSPObject*
      virtual void set(const std::string &name, OSPObject v) const = 0;

      // ManagedObject&
      virtual void set(const std::string &name, const ManagedObject &v) const = 0;

      // Remove parameter on the object
      virtual void remove(const std::string &name) const = 0;

      //! Commit to ospray
      virtual void commit() const = 0;

      //! Release the handle, sets the held handle instance to 'nullptr'
      virtual void release() = 0;

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
      virtual ~ManagedObject_T() override;

      void set(const std::string &name, const std::string &v) const override;

      void set(const std::string &name, int v) const override;
      void set(const std::string &name, int v1, int v2) const override;
      void set(const std::string &name, int v1, int v2, int v3) const override;

      void set(const std::string &name, float v) const override;
      void set(const std::string &name, float v1, float v2) const override;
      void set(const std::string &name, float v1, float v2, float v3) const override;
      void set(const std::string &name, float v1, float v2, float v3, float v4) const override;

      void set(const std::string &name, double v) const override;
      void set(const std::string &name, double v1, double v2) const override;
      void set(const std::string &name, double v1, double v2, double v3) const override;
      void set(const std::string &name, double v1, double v2, double v3, double v4) const override;

      void set(const std::string &name, const ospcommon::vec2i &v) const override;
      void set(const std::string &name, const ospcommon::vec2f &v) const override;

      void set(const std::string &name, const ospcommon::vec3i &v) const override;
      void set(const std::string &name, const ospcommon::vec3f &v) const override;

      void set(const std::string &name, const ospcommon::vec4f &v) const override;

      void set(const std::string &name, const char *v) const override;

      void set(const std::string &name, void *v) const override;

      void set(const std::string &name, OSPObject v) const override;

      void set(const std::string &name, const ManagedObject &v) const override;

      void remove(const std::string &name) const override;

      void commit() const override;

      void release() override;

      OSPObject object() const override;

      //! Get the underlying specific OSP* handle
      OSP_TYPE handle() const;

      //! return whether the given object is valid, or NULL
      inline operator bool () const { return handle() != nullptr; }

    protected:

      OSP_TYPE ospObject;
    };

    // Inlined function definitions ///////////////////////////////////////////

    template <typename OSP_TYPE>
    inline ManagedObject_T<OSP_TYPE>::ManagedObject_T(OSP_TYPE object) :
      ospObject(object)
    {
      using OSPObject_T = typename std::remove_pointer<OSPObject>::type;
      using OtherOSP_T  = typename std::remove_pointer<OSP_TYPE>::type;
      static_assert(std::is_same<osp::ManagedObject, OSPObject_T>::value ||
                    std::is_base_of<osp::ManagedObject, OtherOSP_T>::value,
                    "ManagedObject_T<OSP_TYPE> can only be instantiated with "
                    "OSPObject (a.k.a. osp::ManagedObject*) or one of its"
                    "descendants (a.k.a. the OSP* family of types).");
    }

    template <typename OSP_TYPE>
    inline ManagedObject_T<OSP_TYPE>::~ManagedObject_T()
    {
    }

    template <typename OSP_TYPE>
    inline void ManagedObject_T<OSP_TYPE>::set(const std::string &name,
                                               const std::string &v) const
    {
      ospSetString(ospObject, name.c_str(), v.c_str());
    }

    template <typename OSP_TYPE>
    inline void ManagedObject_T<OSP_TYPE>::set(const std::string &name, int v) const
    {
      ospSet1i(ospObject, name.c_str(), v);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject_T<OSP_TYPE>::set(const std::string &name,
                                               int v1, int v2) const
    {
      ospSet2i(ospObject, name.c_str(), v1, v2);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject_T<OSP_TYPE>::set(const std::string &name,
                                               int v1, int v2, int v3) const
    {
      ospSet3i(ospObject, name.c_str(), v1, v2, v3);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject_T<OSP_TYPE>::set(const std::string &name, float v) const
    {
      ospSet1f(ospObject, name.c_str(), v);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject_T<OSP_TYPE>::set(const std::string &name,
                                               float v1, float v2) const
    {
      ospSet2f(ospObject, name.c_str(), v1, v2);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject_T<OSP_TYPE>::set(const std::string &name,
                                               float v1, float v2, float v3) const
    {
      ospSet3f(ospObject, name.c_str(), v1, v2, v3);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject_T<OSP_TYPE>::set(const std::string &name,
                                               float v1, float v2,
                                               float v3, float v4) const
    {
      ospSet4f(ospObject, name.c_str(), v1, v2, v3, v4);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject_T<OSP_TYPE>::set(const std::string &name, double v) const
    {
      ospSet1f(ospObject, name.c_str(), v);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject_T<OSP_TYPE>::set(const std::string &name,
                                               double v1, double v2) const
    {
      ospSet2f(ospObject, name.c_str(), v1, v2);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject_T<OSP_TYPE>::set(const std::string &name,
                                               double v1, double v2, double v3) const
    {
      ospSet3f(ospObject, name.c_str(), v1, v2, v3);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject_T<OSP_TYPE>::set(const std::string &name,
                                               double v1, double v2,
                                               double v3, double v4) const
    {
      ospSet4f(ospObject, name.c_str(), v1, v2, v3, v4);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject_T<OSP_TYPE>::set(const std::string &name,
                                               const ospcommon::vec2i &v) const
    {
      ospSetVec2i(ospObject, name.c_str(), (const osp::vec2i&)v);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject_T<OSP_TYPE>::set(const std::string &name,
                                               const ospcommon::vec2f &v) const
    {
      ospSetVec2f(ospObject, name.c_str(), (const osp::vec2f&)v);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject_T<OSP_TYPE>::set(const std::string &name,
                                               const ospcommon::vec3i &v) const
    {
      ospSetVec3i(ospObject, name.c_str(), (const osp::vec3i&)v);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject_T<OSP_TYPE>::set(const std::string &name,
                                               const ospcommon::vec3f &v) const
    {
      ospSetVec3f(ospObject, name.c_str(), (const osp::vec3f&)v);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject_T<OSP_TYPE>::set(const std::string &name,
                                               const ospcommon::vec4f &v) const
    {
      ospSetVec4f(ospObject, name.c_str(), (const osp::vec4f&)v);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject_T<OSP_TYPE>::set(const std::string &name, const char *v) const
    {
      ospSetString(ospObject, name.c_str(), v);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject_T<OSP_TYPE>::set(const std::string &name, void *v) const
    {
      ospSetVoidPtr(ospObject, name.c_str(), v);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject_T<OSP_TYPE>::set(const std::string &name, OSPObject v) const
    {
      ospSetObject(ospObject, name.c_str(), v);
    }

    template <typename OSP_TYPE>
    inline void
    ManagedObject_T<OSP_TYPE>::set(const std::string &name, const ManagedObject &v) const
    {
      ospSetObject(ospObject, name.c_str(), v.object());
    }

    template <typename OSP_TYPE>
    inline void ManagedObject_T<OSP_TYPE>::remove(const std::string &name) const
    {
      ospRemoveParam(ospObject, name.c_str());
    }

    template <typename OSP_TYPE>
    inline void ManagedObject_T<OSP_TYPE>::commit() const
    {
      ospCommit(ospObject);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject_T<OSP_TYPE>::release()
    {
      ospRelease(ospObject);
      ospObject = nullptr;
    }

    template <typename OSP_TYPE>
    OSPObject ManagedObject_T<OSP_TYPE>::object() const
    {
      return (OSPObject)ospObject;
    }

    template <typename OSP_TYPE>
    inline OSP_TYPE ManagedObject_T<OSP_TYPE>::handle() const
    {
      return ospObject;
    }

  }// namespace cpp
}// namespace ospray
