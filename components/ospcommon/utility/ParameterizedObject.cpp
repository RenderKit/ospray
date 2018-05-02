// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

#include "ParameterizedObject.h"

#include <algorithm>

namespace ospcommon {
  namespace utility {

    ParameterizedObject::Param::Param(const std::string &_name) : name(_name)
    {
    }

    void ParameterizedObject::removeParam(const std::string &name)
    {
      auto foundParam =
          std::find_if(paramList.begin(), paramList.end(),
            [&](const std::shared_ptr<Param> &p) {
              return p->name == name;
            });

      if (foundParam != paramList.end()) {
        paramList.erase(foundParam);
      }
    }

    ParameterizedObject::Param *
    ParameterizedObject::findParam(const std::string &name, bool addIfNotExist)
    {
      auto foundParam =
          std::find_if(paramList.begin(), paramList.end(),
            [&](const std::shared_ptr<Param> &p) {
              return p->name == name;
            });

      if (foundParam != paramList.end())
        return foundParam->get();
      else if (addIfNotExist) {
        paramList.push_back(std::make_shared<Param>(name));
        return paramList[paramList.size()-1].get();
      }
      else
        return nullptr;
    }

    std::shared_ptr<ParameterizedObject::Param> *
    ParameterizedObject::params_begin()
    {
      return &paramList[0];
    }

    std::shared_ptr<ParameterizedObject::Param> *
    ParameterizedObject::params_end()
    {
      return params_begin() + paramList.size();
    }

  } // ::ospcommon::utility
} // ::ospcommon
