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

#include "Loc.h"
#include <stdarg.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

namespace ospray {
  Loc Loc::current;

  string Loc::toString() const { 
    char buffer[10000];
    sprintf(buffer," @ [%s:%d] ", name, line); 
    return buffer;
  }

  static void lPrint(const char *type, Loc p, const char *fmt, va_list args) {
#ifdef VOLTA_IS_WINDOWS
    char errorBuf[2048];
    if (vsnprintf_s(errorBuf, sizeof(errorBuf), _TRUNCATE, fmt, args) == -1) {
#else
      char *errorBuf;
      if (vasprintf(&errorBuf, fmt, args) == -1) {
#endif
        fprintf(stderr, "vasprintf() unable to allocate memory!\n");
        abort();
      }

      fprintf(stderr, "%s(%d): %s: %s\n", p.name, p.line, type, errorBuf);
#ifndef VOLTA_IS_WINDOWS
      free(errorBuf);
#endif // !VOLTA_IS_WINDOWS
    }


    void Error(Loc p, const char *fmt, ...) {
      std::cout << "=======================================================" << std::endl;
      va_list args;
      va_start(args, fmt);
      lPrint("\nError", p, fmt, args);
      va_end(args);
      std::cout << "=======================================================" << std::endl;
      assert(0);
      exit(1);
    }


    void Warning(Loc p, const char *fmt, ...) {
      va_list args;
      va_start(args, fmt);
      lPrint("Warning", p, fmt, args);
      va_end(args);
    }


    void PerformanceNotice(Loc p, const char *fmt, ...) {
      va_list args;
      va_start(args, fmt);
      lPrint("Performance Notice", p, fmt, args);
      va_end(args);
    }


    void PerformanceWarning(Loc p, const char *fmt, ...) {
      va_list args;
      va_start(args, fmt);
      lPrint("Performance Warning", p, fmt, args);
      va_end(args);
    }
  }

