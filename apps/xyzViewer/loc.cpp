#include "loc.h"
#include <stdarg.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

namespace ospray {
  namespace lammps {
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

  }
