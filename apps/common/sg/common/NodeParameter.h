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

#include "Node.h"

namespace ospray {
  namespace sg {

    // Helper functions ///////////////////////////////////////////////////////

    // Compare //

    template <typename T>
    inline bool valueInRange(const Any& min, const Any& max, const Any& value)
    {
      if (value.get<T>() < min.get<T>() || value.get<T>() > max.get<T>())
        return false;
      return true;
    }

    template <>
    inline bool valueInRange<vec2f>(const Any& min,
                                    const Any& max,
                                    const Any& value)
    {
      const vec2f &v1 = min.get<vec2f>();
      const vec2f &v2 = max.get<vec2f>();
      const vec2f &v  = value.get<vec2f>();
      return !(v1.x > v.x || v2.x < v.x ||
               v1.y > v.y || v2.y < v.y);
    }

    template <>
    inline bool valueInRange<vec2i>(const Any& min,
                                    const Any& max,
                                    const Any& value)
    {
      const vec2i &v1 = min.get<vec2i>();
      const vec2i &v2 = max.get<vec2i>();
      const vec2i &v  = value.get<vec2i>();
      return !(v1.x > v.x || v2.x < v.x ||
               v1.y > v.y || v2.y < v.y);
    }

    template <>
    inline bool valueInRange<vec3f>(const Any& min,
                                    const Any& max,
                                    const Any& value)
    {
      const vec3f &v1 = min.get<vec3f>();
      const vec3f &v2 = max.get<vec3f>();
      const vec3f &v  = value.get<vec3f>();
      return !(v1.x > v.x || v2.x < v.x ||
               v1.y > v.y || v2.y < v.y ||
               v1.z > v.z || v2.z < v.z);
    }

    template <>
    inline bool valueInRange<vec3i>(const Any& min,
                                    const Any& max,
                                    const Any& value)
    {
      const vec3i &v1 = min.get<vec3i>();
      const vec3i &v2 = max.get<vec3i>();
      const vec3i &v  = value.get<vec3i>();
      return !(v1.x > v.x || v2.x < v.x ||
               v1.y > v.y || v2.y < v.y ||
               v1.z > v.z || v2.z < v.z);
    }

    template <>
    inline bool valueInRange<vec3fa>(const Any& min,
                                     const Any& max,
                                     const Any& value)
    {
      const vec3fa &v1 = min.get<vec3fa>();
      const vec3fa &v2 = max.get<vec3fa>();
      const vec3fa &v  = value.get<vec3fa>();
      return !(v1.x > v.x || v2.x < v.x ||
               v1.y > v.y || v2.y < v.y ||
               v1.z > v.z || v2.z < v.z);
    }

    template <>
    inline bool valueInRange<vec4f>(const Any& min,
                                    const Any& max,
                                    const Any& value)
    {
      const vec4f &v1 = min.get<vec4f>();
      const vec4f &v2 = max.get<vec4f>();
      const vec4f &v  = value.get<vec4f>();
      return !(v1.x > v.x || v2.x < v.x ||
               v1.y > v.y || v2.y < v.y ||
               v1.z > v.z || v2.z < v.z ||
               v1.w > v.w || v2.w < v.w);
    }

    template <>
    inline bool valueInRange<box2f>(const Any&, const Any&, const Any&)
    {
      return true;// NOTE(jda) - this is wrong, was incorrect before refactoring
    }

    template <>
    inline bool valueInRange<box2i>(const Any&, const Any&, const Any&)
    {
      return true;// NOTE(jda) - this is wrong, was incorrect before refactoring
    }

    template <>
    inline bool valueInRange<box3f>(const Any&, const Any&, const Any&)
    {
      return true;// NOTE(jda) - this is wrong, was incorrect before refactoring
    }

    template <>
    inline bool valueInRange<box3i>(const Any&, const Any&, const Any&)
    {
      return true;// NOTE(jda) - this is wrong, was incorrect before refactoring
    }

    // Commit //

    template <typename T>
    inline void commitNodeValue(Node &)
    {
    }

    template <>
    inline void commitNodeValue<bool>(Node &n)
    {
      ospSet1i(n.parent().valueAs<OSPObject>(),
               n.name().c_str(), n.valueAs<bool>());
    }

    template <>
    inline void commitNodeValue<int>(Node &n)
    {
      ospSet1i(n.parent().valueAs<OSPObject>(),
               n.name().c_str(), n.valueAs<int>());
    }

    template <>
    inline void commitNodeValue<float>(Node &n)
    {
      ospSet1f(n.parent().valueAs<OSPObject>(),
               n.name().c_str(), n.valueAs<float>());
    }

    template <>
    inline void commitNodeValue<vec2f>(Node &n)
    {
      ospSet2fv(n.parent().valueAs<OSPObject>(),
                n.name().c_str(), &n.valueAs<vec2f>().x);
    }

    template <>
    inline void commitNodeValue<vec2i>(Node &n)
    {
      ospSet2iv(n.parent().valueAs<OSPObject>(),
                n.name().c_str(), &n.valueAs<vec2i>().x);
    }

    template <>
    inline void commitNodeValue<vec3f>(Node &n)
    {
      ospSet3fv(n.parent().valueAs<OSPObject>(),
                n.name().c_str(), &n.valueAs<vec3f>().x);
    }

    template <>
    inline void commitNodeValue<vec3i>(Node &n)
    {
      ospSet3iv(n.parent().valueAs<OSPObject>(),
                n.name().c_str(), &n.valueAs<vec3i>().x);
    }

    template <>
    inline void commitNodeValue<vec3fa>(Node &n)
    {
      ospSet3fv(n.parent().valueAs<OSPObject>(),
                n.name().c_str(), &n.valueAs<vec3fa>().x);
    }

    template <>
    inline void commitNodeValue<vec4f>(Node &n)
    {
      ospSet4fv(n.parent().valueAs<OSPObject>(),
                n.name().c_str(), &n.valueAs<vec4f>().x);
    }

    template <>
    inline void commitNodeValue<box2f>(Node &n)
    {
      auto parent = n.parent().valueAs<OSPObject>();
      auto name   = n.name();
      auto value  = n.valueAs<box2f>();

      ospSet2fv(parent, (name + ".upper").c_str(), &value.upper.x);
      ospSet2fv(parent, (name + ".lower").c_str(), &value.lower.x);
    }

    template <>
    inline void commitNodeValue<box2i>(Node &n)
    {
      auto parent = n.parent().valueAs<OSPObject>();
      auto name   = n.name();
      auto value  = n.valueAs<box2i>();

      ospSet2iv(parent, (name + ".upper").c_str(), &value.upper.x);
      ospSet2iv(parent, (name + ".lower").c_str(), &value.lower.x);
    }

    template <>
    inline void commitNodeValue<box3f>(Node &n)
    {
      auto parent = n.parent().valueAs<OSPObject>();
      auto name   = n.name();
      auto value  = n.valueAs<box3f>();

      ospSet3fv(parent, (name + ".upper").c_str(), &value.upper.x);
      ospSet3fv(parent, (name + ".lower").c_str(), &value.lower.x);
    }

    template <>
    inline void commitNodeValue<box3i>(Node &n)
    {
      auto parent = n.parent().valueAs<OSPObject>();
      auto name   = n.name();
      auto value  = n.valueAs<box3i>();

      ospSet3iv(parent, (name + ".upper").c_str(), &value.upper.x);
      ospSet3iv(parent, (name + ".lower").c_str(), &value.lower.x);
    }

    template <>
    inline void commitNodeValue<std::string>(Node &n)
    {
      ospSetString(n.parent().valueAs<OSPObject>(),
                   n.name().c_str(), n.valueAs<std::string>().c_str());
    }

    // Helper parameter node wrapper //////////////////////////////////////////

    template <typename T>
    struct NodeParam : public Node
    {
      NodeParam() : Node() { setValue(T()); }
      virtual void postCommit(RenderContext &) override
      {
        if (hasParent()) {
          //TODO: generalize to other types of ManagedObject

          //NOTE(jda) - OMG the syntax for the 'if' is strange...
          if (parent().value().template is<OSPObject>())
            commitNodeValue<T>(*this);
        }
      }

      virtual bool computeValidMinMax() override
      {
        if (properties.minmax.size() < 2 ||
            !(flags() & NodeFlags::valid_min_max))
          return true;

        return valueInRange<T>(min(), max(), value());
      }
    };

  } // ::ospray::sg
} // ::ospray