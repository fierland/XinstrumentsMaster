/*
 * Simple header defining macros for debug output_iterator
 */

#ifndef DEBUG_MACROS_H_
#define DEBUG_MACROS_H_

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#define NO_DEBUG 0
#define MACRO_DEBUG  //If you comment this line, the DPRINT & DPRINTLN lines are defined as blank.
//#define MACRO_DEBUG_DETAIL //If you comment this line, the D1PRINT & D1PRINTLN lines are defined as blank.

#ifdef MACRO_DEBUG    //Macros are usually in all capital letters.
#define DPRINT(...)    	Serial.print(__VA_ARGS__)    //DPRINT is a macro, debug print
#define DPRINTLN(...)  	Serial.println(__VA_ARGS__)   //DPRINTLN is a macro, debug print with new line
#define DPRINTBUFFER(x,y) DPRINT("Buffer:");for(int i=0;i<y;i++){DPRINT(x[i]);DPRINT(":");}	DPRINTLN("END");
#define DPRINTINFO(...)    \
   Serial.print(millis());     \
   Serial.print(": ");    \
   Serial.print(__PRETTY_FUNCTION__); \
   Serial.print(' ');      \
   Serial.print(__FILE__);     \
   Serial.print(':');      \
   Serial.print(__LINE__);     \
   Serial.print(' ');      \
   Serial.println(__VA_ARGS__);
#else
#define DPRINT(...)     //now defines a blank line
#define DPRINTLN(...)   //now defines a blank line
#define DPRINTBUFFER(x,y)
#define DPRINTINFO(...)
#endif

#ifdef MACRO_DEBUG_DETAIL    //Macros are usually in all capital letters.
#define D1PRINT(...)    	Serial.print(__VA_ARGS__)    //DPRINT is a macro, debug print
#define D1PRINTLN(...)  	Serial.println(__VA_ARGS__)   //DPRINTLN is a macro, debug print with new line
#define D1PRINTBUFFER(x,y) DPRINT("Buffer:");for(int i=0;i<y;i++){DPRINT(x[i]);DPRINT(":");}	DPRINTLN("END");
#define D1PRINTINFO(...)    \
   Serial.print(millis());     \
   Serial.print(": ");    \
   Serial.print(__PRETTY_FUNCTION__); \
   Serial.print(' ');      \
   Serial.print(__FILE__);     \
   Serial.print(':');      \
   Serial.print(__LINE__);     \
   Serial.print(' ');      \
   Serial.println(__VA_ARGS__);
#else
#define D1PRINT(...)     //now defines a blank line
#define D1PRINTLN(...)   //now defines a blank line
#define D1PRINTBUFFER(x,y)
#define D1PRINTINFO(...)
#endif

#endif
