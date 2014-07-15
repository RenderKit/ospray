/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

#include "model.h"
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
}



