// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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

/*! \file PseudoURL: splits a 'pseudo-url' of the form
  '<type>://<filename>[:name=value]*' into its components of 'type'
    (e.g, 'points', 'lines', etc), filename, and 'name=value'
    argument pairs (e.g., 'format=xyzrgb') */

#include "PseudoURL.h"

namespace ospcommon {
  namespace utility {

    void tokenize(const std::string &str, const char delim,
                  std::vector<std::string> &tokens)
    {
      size_t prev = 0;
      size_t fnd = str.find(delim);
      for (; fnd != std::string::npos;  prev = fnd + 1, fnd = str.find(delim, prev)) {
        // Discard repeated tokens in the string, e.g. tokeninzing a::c::b on ':' should
        // just return a, c, b
        if (fnd - prev > 1) {
          tokens.push_back(str.substr(prev, fnd - prev));
        }
      }
      // Grab the last token in the string, if the string didn't terminate with a delimeter
      if (str.size() - prev > 1) {
        tokens.push_back(str.substr(prev));
      }
    }

    /*! constructor - parse the given string into its components */
    PseudoURL::PseudoURL(const std::string &inputString)
    {
      std::string tmp = inputString;
      const size_t separator = tmp.find("://");
      if (separator != std::string::npos) {
        // separator specified: cut off 'type' before that separator,
        // and reset 'tmp' to everything behind it
        type = tmp.substr(0, separator);
        tmp  = tmp.substr(separator + 3);
      } else {
        // no separator -> empty type specifier string, tmp returns
        // un-modified
        type = "";
      }

      /* now, split remainder into its colon-separated components (the
         first of those is the filename, all other ones are params */
      std::vector<std::string> colonSeparatedComponents;
      tokenize(tmp, ':', colonSeparatedComponents);

      if (colonSeparatedComponents.empty())
        // degenerate case of "type://<nothing>" - return empty filename and empty params
        return;

      fileName = colonSeparatedComponents[0];
      for (size_t arg_it = 1; arg_it < colonSeparatedComponents.size(); arg_it++) {
        std::string arg = colonSeparatedComponents[arg_it];
        const size_t equalSign = arg.find('=');
        if (equalSign != std::string::npos) {
          params.push_back(std::make_pair(arg.substr(0, equalSign),
                                          arg.substr(equalSign+1)));
        } else {
          params.push_back(std::make_pair(arg, std::string("")));
        }
      }
    }

    /*! return the parsed type. may we empty string if none was specified */
    std::string PseudoURL::getType() const
    {
      return type;
    }

    /*! return the parsed file name specifier. cannot be empty
      string */
    std::string PseudoURL::getFileName() const
    {
      return fileName;
    }

    /*! return value for given parameters name, or throw an exception
      if not specified */
    std::string PseudoURL::getValue(const std::string &name) const
    {
      /* note(iw) - we do _not_ do a immediate 'return' upon the first
         param with mathcin gname we find so as to ensure that we use
         the _last_ time any parameter was written. it's more intuitive
         to have the last value override earlier ones, but i didn't want
         the parser (ie, constructor) to mess with the input data (maybe
         in some cases a class using this _wants_ to have multiple
         instances of the same parameter!?), so let's fix that here */
      int found = -1;
      for (size_t i = 0; i < params.size(); i++) {
        if (params[i].first == name)
          found = i;
      }

      if (found < 0) {
        throw std::runtime_error("PseudoURL::getValue queried value of "
                                 "not-specified parameter");
      }

      return params[found].second;
    }

    /*! check if the given parameter was specified */
    bool PseudoURL::hasParam(const std::string &name)
    {
      for (auto param : params)
        if (param.first == name) return true;
      return false;
    }

  } // ::ospcommon::utility
} // ::ospcommon
