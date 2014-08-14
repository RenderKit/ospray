#include "ospcommon.ih"

//embree
#include "embree2/rtcore.isph"

//extern "C" void abort ();
//extern "C" void exit(uniform int);
//extern "C" uniform int puts ( const uniform int8* uniform str );
//extern "C" uniform int putchar ( uniform int character );

void error_handler(const RTCError code, const int8* str)
{
  print("Embree: ");
  switch (code) {
  case RTC_UNKNOWN_ERROR    : print("RTC_UNKNOWN_ERROR"); break;
  case RTC_INVALID_ARGUMENT : print("RTC_INVALID_ARGUMENT"); break;
  case RTC_INVALID_OPERATION: print("RTC_INVALID_OPERATION"); break;
  case RTC_OUT_OF_MEMORY    : print("RTC_OUT_OF_MEMORY"); break;
  case RTC_UNSUPPORTED_CPU  : print("RTC_UNSUPPORTED_CPU"); break;
  default                   : print("invalid error code"); break;
  }
  if (str) { 
    print("(%)", str);
    //print(" ("); 
    //while (*str) putchar(*str++); 
    //print(")\n"); 
  }
  assert(0);
}
