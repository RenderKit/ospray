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

#include "ospray/ospray_util.h"

#include "ospcommon/math/AffineSpace.h"
#include "ospcommon/math/vec.h"

namespace ospray {
  namespace cpp {

    using namespace ospcommon::math;

    template <typename OSP_TYPE = OSPObject>
    class ManagedObject
    {
     public:
      ManagedObject(OSP_TYPE object = nullptr);
      ~ManagedObject();

      ManagedObject(const ManagedObject<OSP_TYPE> &copy);
      ManagedObject(ManagedObject<OSP_TYPE> &&move);

      ManagedObject &operator=(const ManagedObject<OSP_TYPE> &copy);
      ManagedObject &operator=(ManagedObject<OSP_TYPE> &&move);

      void setParam(const std::string &name, const std::string &v) const;

      void setParam(const std::string &name, bool v) const;
      void setParam(const std::string &name, int v) const;
      void setParam(const std::string &name, float v) const;
      void setParam(const std::string &name, double v) const;

      void setParam(const std::string &name, const vec2i &v) const;
      void setParam(const std::string &name, const vec2f &v) const;

      void setParam(const std::string &name, const vec3i &v) const;
      void setParam(const std::string &name, const vec3f &v) const;

      void setParam(const std::string &name, const vec4i &v) const;
      void setParam(const std::string &name, const vec4f &v) const;

      void setParam(const std::string &name, const box1i &v) const;
      void setParam(const std::string &name, const box1f &v) const;

      void setParam(const std::string &name, const box2i &v) const;
      void setParam(const std::string &name, const box2f &v) const;

      void setParam(const std::string &name, const box3i &v) const;
      void setParam(const std::string &name, const box3f &v) const;

      void setParam(const std::string &name, const box4i &v) const;
      void setParam(const std::string &name, const box4f &v) const;

      void setParam(const std::string &name, const linear3f &v) const;
      void setParam(const std::string &name, const affine3f &v) const;

      void setParam(const std::string &name, const char *v) const;

      void setParam(const std::string &name, void *v) const;

      void setParam(const std::string &name, OSPObject v) const;

      template <typename OT>
      void setParam(const std::string &name,
                    const ManagedObject<OT> &v) const;

      void removeParam(const std::string &name) const;

      box3f getBounds() const;

      void commit() const;

      OSPObject object() const;

      //! Get the underlying specific OSP* handle
      OSP_TYPE handle() const;

      //! return whether the given object is valid, or NULL
      operator bool() const;

     protected:
      OSP_TYPE ospObject;
    };

    // Inlined function definitions ///////////////////////////////////////////

    template <typename OSP_TYPE>
    inline ManagedObject<OSP_TYPE>::ManagedObject(OSP_TYPE object)
        : ospObject(object)
    {
      using OSPObject_T = typename std::remove_pointer<OSPObject>::type;
      using OtherOSP_T  = typename std::remove_pointer<OSP_TYPE>::type;
      static_assert(std::is_same<osp::ManagedObject, OSPObject_T>::value ||
                        std::is_base_of<osp::ManagedObject, OtherOSP_T>::value,
                    "ManagedObject<OSP_TYPE> can only be instantiated with "
                    "OSPObject (a.k.a. osp::ManagedObject*) or one of its"
                    "descendants (a.k.a. the OSP* family of types).");
    }

    template <typename OSP_TYPE>
    inline ManagedObject<OSP_TYPE>::~ManagedObject()
    {
      ospRelease(ospObject);
    }

    template <typename OSP_TYPE>
    inline ManagedObject<OSP_TYPE>::ManagedObject(
        const ManagedObject<OSP_TYPE> &copy)
    {
      ospObject = copy.handle();
      ospRetain(copy.handle());
    }

    template <typename OSP_TYPE>
    inline ManagedObject<OSP_TYPE>::ManagedObject(
        ManagedObject<OSP_TYPE> &&move)
    {
      ospObject      = move.handle();
      move.ospObject = nullptr;
    }

    template <typename OSP_TYPE>
    inline ManagedObject<OSP_TYPE> &ManagedObject<OSP_TYPE>::operator=(
        const ManagedObject<OSP_TYPE> &copy)
    {
      ospObject = copy.handle();
      ospRetain(copy.handle());
      return *this;
    }

    template <typename OSP_TYPE>
    inline ManagedObject<OSP_TYPE> &ManagedObject<OSP_TYPE>::operator=(
        ManagedObject<OSP_TYPE> &&move)
    {
      ospObject      = move.handle();
      move.ospObject = nullptr;
      return *this;
    }

    template <typename OSP_TYPE>
    inline void ManagedObject<OSP_TYPE>::setParam(const std::string &name,
                                                    const std::string &v) const
    {
      ospSetParam(ospObject, name.c_str(), OSP_STRING, v.c_str());
    }

    template <typename OSP_TYPE>
    inline void ManagedObject<OSP_TYPE>::setParam(const std::string &name,
                                                    bool v) const
    {
      ospSetBool(ospObject, name.c_str(), v);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject<OSP_TYPE>::setParam(const std::string &name,
                                                    int v) const
    {
      ospSetInt(ospObject, name.c_str(), v);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject<OSP_TYPE>::setParam(const std::string &name,
                                                    float v) const
    {
      ospSetFloat(ospObject, name.c_str(), v);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject<OSP_TYPE>::setParam(const std::string &name,
                                                    double v) const
    {
      ospSetFloat(ospObject, name.c_str(), v);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject<OSP_TYPE>::setParam(const std::string &name,
                                                    const vec2i &v) const
    {
      ospSetParam(ospObject, name.c_str(), OSP_VEC2I, &v.x);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject<OSP_TYPE>::setParam(const std::string &name,
                                                    const vec2f &v) const
    {
      ospSetParam(ospObject, name.c_str(), OSP_VEC2F, &v.x);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject<OSP_TYPE>::setParam(const std::string &name,
                                                    const vec3i &v) const
    {
      ospSetParam(ospObject, name.c_str(), OSP_VEC3I, &v.x);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject<OSP_TYPE>::setParam(const std::string &name,
                                                    const vec3f &v) const
    {
      ospSetParam(ospObject, name.c_str(), OSP_VEC3F, &v.x);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject<OSP_TYPE>::setParam(const std::string &name,
                                                    const vec4i &v) const
    {
      ospSetParam(ospObject, name.c_str(), OSP_VEC4I, &v.x);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject<OSP_TYPE>::setParam(const std::string &name,
                                                    const vec4f &v) const
    {
      ospSetParam(ospObject, name.c_str(), OSP_VEC4F, &v.x);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject<OSP_TYPE>::setParam(const std::string &name,
                                                    const box1i &v) const
    {
      ospSetParam(ospObject, name.c_str(), OSP_BOX1I, &v.lower);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject<OSP_TYPE>::setParam(const std::string &name,
                                                    const box1f &v) const
    {
      ospSetParam(ospObject, name.c_str(), OSP_BOX1F, &v.lower);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject<OSP_TYPE>::setParam(const std::string &name,
                                                    const box2i &v) const
    {
      ospSetParam(ospObject, name.c_str(), OSP_BOX2I, &v.lower.x);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject<OSP_TYPE>::setParam(const std::string &name,
                                                    const box2f &v) const
    {
      ospSetParam(ospObject, name.c_str(), OSP_BOX2F, &v.lower.x);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject<OSP_TYPE>::setParam(const std::string &name,
                                                    const box3i &v) const
    {
      ospSetParam(ospObject, name.c_str(), OSP_BOX3I, &v.lower.x);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject<OSP_TYPE>::setParam(const std::string &name,
                                                    const box3f &v) const
    {
      ospSetParam(ospObject, name.c_str(), OSP_BOX3F, &v.lower.x);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject<OSP_TYPE>::setParam(const std::string &name,
                                                    const box4i &v) const
    {
      ospSetParam(ospObject, name.c_str(), OSP_BOX4I, &v.lower.x);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject<OSP_TYPE>::setParam(const std::string &name,
                                                    const box4f &v) const
    {
      ospSetParam(ospObject, name.c_str(), OSP_BOX4F, &v.lower.x);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject<OSP_TYPE>::setParam(const std::string &name,
                                                    const linear3f &v) const
    {
      ospSetParam(ospObject, name.c_str(), OSP_LINEAR3F, (const float *)&v);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject<OSP_TYPE>::setParam(const std::string &name,
                                                    const affine3f &v) const
    {
      ospSetParam(ospObject, name.c_str(), OSP_AFFINE3F, (const float *)&v);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject<OSP_TYPE>::setParam(const std::string &name,
                                                    const char *v) const
    {
      ospSetParam(ospObject, name.c_str(), OSP_STRING, v);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject<OSP_TYPE>::setParam(const std::string &name,
                                                    void *v) const
    {
      ospSetParam(ospObject, name.c_str(), OSP_VOID_PTR, v);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject<OSP_TYPE>::setParam(const std::string &name,
                                                    OSPObject v) const
    {
      ospSetObject(ospObject, name.c_str(), v);
    }

    template <typename OSP_TYPE>
    template <typename OT>
    inline void ManagedObject<OSP_TYPE>::setParam(
        const std::string &name, const ManagedObject<OT> &v) const
    {
      ospSetObject(ospObject, name.c_str(), v.object());
    }

    template <typename OSP_TYPE>
    inline void ManagedObject<OSP_TYPE>::removeParam(
        const std::string &name) const
    {
      ospRemoveParam(ospObject, name.c_str());
    }

    template <typename OSP_TYPE>
    box3f ManagedObject<OSP_TYPE>::getBounds() const
    {
      auto b = ospGetBounds(ospObject);
      return box3f(b.lower);
    }

    template <typename OSP_TYPE>
    inline void ManagedObject<OSP_TYPE>::commit() const
    {
      ospCommit(ospObject);
    }

    template <typename OSP_TYPE>
    OSPObject ManagedObject<OSP_TYPE>::object() const
    {
      return (OSPObject)ospObject;
    }

    template <typename OSP_TYPE>
    inline OSP_TYPE ManagedObject<OSP_TYPE>::handle() const
    {
      return ospObject;
    }

    template <typename OSP_TYPE>
    inline ManagedObject<OSP_TYPE>::operator bool() const
    {
      return handle() != nullptr;
    }

  }  // namespace cpp
}  // namespace ospray
