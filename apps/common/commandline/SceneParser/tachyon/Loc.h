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

// tachyon module
#include "Model.h"
// std
#include <string>
#include <ostream>

namespace ospray {
  using std::string;

  struct Loc {
    Loc() { name = "(undef/internal)"; line = 0; }
    const char *name;
    int line;
    void Print() const { printf(" @ [%s:%d] ", name, line); }
    string toString() const;
  
    static Loc current;
  };

  inline std::ostream &operator<<(std::ostream &o, const Loc &fp)
  {
    o << fp.toString();
    return o;
  }

  void Warning(Loc p, const char *, ...);
  void Error(Loc p, const char *, ...);
  void PerformanceNotice(Loc p, const char *, ...);
  void PerformanceWarning(Loc p, const char *, ...);

} // ::ospray



