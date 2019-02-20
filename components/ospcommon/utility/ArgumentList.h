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

/*! \file ArgumentList.h Defines an interface for storing - and consuming -
  command line parameters

  The way this is supposed to work is that the app creates an
  arglist from the ac/av parameters passed to it, then individual
  modules can scan through this class, check if they recognize any,
  parse those, and 'consume' them (it, take them off this list),
  thus indicating that those have been properly processed. This in
  particular allows an app to determine if any command lin
  eparameters have _not_ been processed by any modules, which
  typically indicates a user having used a deprecated way of
  specifying an input parameter (or simply, had a typo :-/ ).
*/

#pragma once

#include <string>
#include <vector>

namespace ospcommon {
  namespace utility {

    /*! class that abstracts command line arguments */
    struct ArgumentList
    {
      /*! initialize a new argument list. note that we will _drop_
        av[0], as this isn't actually an argument */
      ArgumentList(int ac, const char **av);

      /*! return (a copy of) the idx'th argument. Note that unlike
        'real' ac/av numbering of the args starts at 0, not 1
        (because we drop the binary name, and only store
        arguments) */
      std::string operator[](const int idx) const;

      /*! return number of arguments still in list */
      int size() const;

      /*! return number of arguments still in list */
      bool empty() const;

      /*! remove given number of arguments at given index in list */
      void remove(int where, int howMany = 1);

    private:
      std::vector<std::string> arg;
    };

    /*! helper abstraction for any class that can wants to arguments -
      rather than having to manually iterate over all arguments, this
      class allows any class that is derived from it to simply call
      ArgsParser::parseAndConsume(), and do all its detection of
      parsing of command line arguments by overriding
      'tryConsume()' */
    struct ArgumentsParser
    {
      virtual ~ArgumentsParser() = default;

      /*! check if given arg is one of ours. if so, consume it (and all
        its successive parameters that depend on it, and return the
        total number of arguments consumed */
      virtual int tryConsume(ArgumentList &argList, int argID) = 0;

      /*! This function goes over an argument list, and calls
        'tryConsume()' for every argument, then takes those that
        _have_ been indicated as 'yes, we rcongized those' from the
        argument list. (usually does not have to be overridden) */
      void parseAndRemove(ArgumentList &args);
    };

    // ------------------------------------------------------------------
    // (header-only) implementatoin section from here on:
    // ------------------------------------------------------------------

    inline ArgumentList::ArgumentList(int ac, const char **av)
    {
      for (int i = 1; i < ac; i++)
        arg.push_back(av[i]);
    }

    inline std::string ArgumentList::operator[](int idx) const
    {
      return arg.at(idx);
    }

    inline int ArgumentList::size() const
    {
      return arg.size();
    }

    inline bool ArgumentList::empty() const
    {
      return arg.empty();
    }

    inline void ArgumentList::remove(int where, int howMany)
    {
      for (int i = 0; i < howMany; i++)
        arg.erase(arg.begin() + where, arg.begin() + where + 1);
    }

    inline void ArgumentsParser::parseAndRemove(ArgumentList &argList)
    {
      for (int argID = 0; argID < argList.size(); /*no-op*/) {
        int numConsumed = tryConsume(argList,argID);
        if (numConsumed == 0)
          ++argID;
        else
          argList.remove(argID, numConsumed);
      }
    }

  } // ::ospcommon::utility
} // ::ospcommon
