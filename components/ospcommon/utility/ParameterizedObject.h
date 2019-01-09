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

// stl
#include <vector>
// ospcommon
#include "Any.h"

namespace ospcommon {
  namespace utility {

    /*! \brief defines a basic object whose lifetime is managed by ospray */
    struct OSPCOMMON_INTERFACE ParameterizedObject
    {
      ParameterizedObject() = default;
      virtual ~ParameterizedObject() = default;

      /*! \brief container for _any_ sort of parameter an app can assign
          to an ospray object */
      struct OSPCOMMON_INTERFACE Param
      {
        Param(const std::string &name);
        ~Param() = default;

        template <typename T>
        void set(const T &v) { data = v; }

        utility::Any data;

        /*! name under which this parameter is registered */
        std::string name;
      };

      /*! \brief check if a given parameter is available */
      bool hasParam(const std::string &name);

      /*! set a parameter with given name to given value, create param if not
       *  existing */
      template<typename T>
      void setParam(const std::string &name, const T &t);

      template<typename T>
      T getParam(const std::string &name, T valIfNotFound);

      void removeParam(const std::string &name);

    protected:

      /*! \brief find a given parameter, or add it if not exists (and so
       *         specified) */
      Param *findParam(const std::string &name, bool addIfNotExist = false);

      /*! enumerate parameters */
      std::shared_ptr<Param> *params_begin();
      std::shared_ptr<Param> *params_end();

    private:

      // Data members //

      /*! \brief list of parameters attached to this object */
      // NOTE(jda) - Use std::shared_ptr because copy/move of a
      //             ParameterizedObject would end up copying parameters, where
      //             destruction of each copy should only result in freeing the
      //             parameters *once*
      std::vector<std::shared_ptr<Param>> paramList;
    };

    // Inlined ParameterizedObject definitions //////////////////////////////////

    inline bool ParameterizedObject::hasParam(const std::string &name)
    {
      return findParam(name, false) != nullptr;
    }

    template<typename T>
    inline void ParameterizedObject::setParam(const std::string &name,
                                              const T &t)
    {
      findParam(name, true)->set(t);
    }

    template<typename T>
    inline T ParameterizedObject::getParam(const std::string &name,
                                           T valIfNotFound)
    {
      Param *param = findParam(name);
      if (!param) return valIfNotFound;
      if (!param->data.is<T>()) return valIfNotFound;
      return param->data.get<T>();
    }

  } // ::ospcommon::utility
} // ::ospcommon
