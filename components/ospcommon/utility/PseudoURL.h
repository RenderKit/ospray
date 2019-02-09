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

/*! \file PseudoURL: splits a 'pseudo-url' of the form
  '<type>://<fileanme>[:name=value]*' into its components of 'type'
    (e.g, 'points', 'slines', etc), filename, and 'name=value'
    argument pairs (e.g., 'format=xyzrgb') */

#include "../common.h"
#include <string>
#include <vector>

namespace ospcommon {
  namespace utility {

    //! \brief Tokenize the string passed on the desired delimiter
    void tokenize(const std::string &str, const char delim,
                  std::vector<std::string> &tokens);

    /* a pseudo-url is of the form '<type>://<filename>[:name=value]*'
       into its components of 'type' (e.g, 'points', 'lines', etc),
       filename, and 'name=value' argument pairs (e.g.,
       'format=xyzrgb'). This class takes a string and splits it into these
       components */
    struct PseudoURL {

      /*! constructor - parse the given string into its components */
      PseudoURL(const std::string &inputString);

      /*! return the parsed type. may we empty string if none was specified */
      std::string getType() const;

      /*! return the parsed file name specifier. cannot be empty
        string */
      std::string getFileName() const;

      /*! return value for given parameters name, or throw an exception
        if not specified */
      std::string getValue(const std::string &name) const;

      /*! check if the given parameter was specified */
      bool hasParam(const std::string &name);

    private:
      /*! the type of the psueod-url, eg, for 'points://file.raw' this
          would be 'points'. If no "://" is specified, this gets set to "" */
      std::string type;
      /*! the filename - the thing after the <type>://, and before the
          ":" that starts parameters */
      std::string fileName;

      /*! the name-value pairs specified as parameters */
      std::vector<std::pair<std::string,std::string>> params;
    };
  
  } // ::ospcommon::utility
} // ::ospcommon
