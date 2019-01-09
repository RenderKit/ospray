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

#include <algorithm>
#include <string>
#include <sstream>
#include <vector>

namespace ospcommon {
  namespace utility {

    /* return a string which is the two inputs match from the beginning of
       each */
    inline std::string longestBeginningMatch(const std::string &first,
                                             const std::string &second)
    {
      // NOTE(jda) - If length of the second string is shorter than the first,
      //             then we can only iterate through the first string the
      //             number of characters of the second string.
      auto maxMatchLength = std::min(first.size(), second.size());
      auto start1 = first.begin();
      auto start2 = second.begin();
      auto end    = first.begin() + maxMatchLength;

      return std::string(
        start1,
        std::mismatch(start1, end, start2).first
      );
    }

    inline bool beginsWith(const std::string &inputString,
                           const std::string &startsWithString)
    {
      auto startingMatch = longestBeginningMatch(inputString, startsWithString);
      return startingMatch.size() == startsWithString.size();
    }

    /* split a string on a single character delimiter */
    inline std::vector<std::string> split(const std::string &input, char delim)
    {
      std::stringstream ss(input);
      std::string item;
      std::vector<std::string> elems;
      while (std::getline(ss, item, delim))
        elems.push_back(std::move(item));
      return elems;
    }

    /* split a string on a set of delimiters */
    inline std::vector<std::string> split(const std::string &input, 
                                          const std::string &delim)
    {
      std::vector<std::string> tokens;
      size_t pos = 0;
      while (1) {
        size_t begin = input.find_first_not_of(delim,pos);
        if (begin == input.npos) return tokens;
        size_t end = input.find_first_of(delim,begin);
        tokens.push_back(input.substr(begin,(end==input.npos)?input.npos:(end-begin)));
        pos = end;
      }
    }

    /* return lower case version of the input string */
    inline std::string lowerCase(const std::string &str)
    {
      std::string retval = str;
      std::transform(retval.begin(), retval.end(), retval.begin(), ::tolower);
      return retval;
    }

    /* return upper case version of the input string */
    inline std::string upperCase(const std::string &str)
    {
      std::string retval = str;
      std::transform(retval.begin(), retval.end(), retval.begin(), ::toupper);
      return retval;
    }

  } // ::ospcommon::utility
} // ::ospcommon
