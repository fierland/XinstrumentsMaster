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
#define DEBUG_LEVEL 3

#if DEBUG_LEVEL > 0

typedef enum {
	LOG_NONE,       /*!< No log output */
	LOG_ERROR,      /*!< Critical errors, software module can not recover on its own */
	LOG_WARN,       /*!< Error conditions from which recovery measures have been taken */
	LOG_INFO,       /*!< Information messages which describe normal flow of events */
	LOG_DEBUG,      /*!< Extra information which is not necessary for normal use (values, pointers, sizes, etc). */
	LOG_VERBOSE     /*!< Bigger chunks of debugging information, or frequent messages which can potentially flood the output. */
} log_level_t;

#define LOG_LOCAL_LEVEL LOG_INFO

#define __SHORT_FILE__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)

#define DO_LOG_LEVEL(level,TAG, format, ...)  {                     \
		char __ch ='I'; \
        if (level==LOG_ERROR )          { __ch='E'; } \
        else if (level==LOG_WARN )      { __ch='W'; } \
        else if (level==LOG_DEBUG )     { __ch='D'; } \
        else if (level==LOG_VERBOSE )   { __ch='V'; }; \
		Serial.printf("%c (%d) %s: ",__ch,millis(),TAG); \
		Serial.printf(format, ##__VA_ARGS__);Serial.println(""); \
    }

#define DO_LOG_LEVEL_LOCAL(level, format, ...)  {               \
        if ( LOG_LOCAL_LEVEL >= level ) DO_LOG_LEVEL(level, format, ##__VA_ARGS__); \
    }

#define DO_LOGE( format, ... ) DO_LOG_LEVEL_LOCAL(LOG_ERROR,	__SHORT_FILE__,format, ##__VA_ARGS__)
#define DO_LOGW( format, ... ) DO_LOG_LEVEL_LOCAL(LOG_WARN,		__SHORT_FILE__,format, ##__VA_ARGS__)
#define DO_LOGI( format, ... ) DO_LOG_LEVEL_LOCAL(LOG_INFO,		__SHORT_FILE__,format, ##__VA_ARGS__)
#define DO_LOGD( format, ... ) DO_LOG_LEVEL_LOCAL(LOG_DEBUG,	__SHORT_FILE__,format, ##__VA_ARGS__)
#define DO_LOGV( format, ... ) DO_LOG_LEVEL_LOCAL(LOG_VERBOSE,	__SHORT_FILE__,format, ##__VA_ARGS__)

#define ESP_EARLY_LOGE(TAG, format, ...) DO_LOG_LEVEL_LOCAL(LOG_ERROR,TAG,format, ##__VA_ARGS__)
#define ESP_EARLY_LOGW(TAG, format, ...) DO_LOG_LEVEL_LOCAL(LOG_WARN,TAG,format, ##__VA_ARGS__)
#define ESP_EARLY_LOGD(TAG, format, ...) DO_LOG_LEVEL_LOCAL(LOG_INFO,TAG,format, ##__VA_ARGS__)
#define ESP_EARLY_LOGV(TAG, format, ...) DO_LOG_LEVEL_LOCAL(LOG_DEBUG,TAG,format, ##__VA_ARGS__)
#define ESP_EARLY_LOGI(TAG, format, ...) DO_LOG_LEVEL_LOCAL(LOG_VERBOSE,TAG,format, ##__VA_ARGS__)

#define ESP_LOGE(TAG, format, ...) DO_LOG_LEVEL_LOCAL(LOG_ERROR,TAG,	format, ##__VA_ARGS__)
#define ESP_LOGW(TAG, format, ...) DO_LOG_LEVEL_LOCAL(LOG_WARN,TAG,	format, ##__VA_ARGS__)
#define ESP_LOGD(TAG, format, ...) DO_LOG_LEVEL_LOCAL(LOG_INFO,TAG,	format, ##__VA_ARGS__)
#define ESP_LOGV(TAG, format, ...) DO_LOG_LEVEL_LOCAL(LOG_DEBUG,TAG,	format, ##__VA_ARGS__)
#define ESP_LOGI(TAG, format, ...) DO_LOG_LEVEL_LOCAL(LOG_VERBOSE,TAG,	format, ##__VA_ARGS__)

#define ESP_ERROR_CHECK(func) {esp_err_t err; err=func; if(err!=ESP_OK)\
	{DO_LOGE("assert failed ERR=%d in %s at line %d\n",err,__PRETTY_FUNCTION__,__LINE__);} }

#define DLVPRINT(level,txt,...) if (DEBUG_LEVEL > level) {Serial.printf(txt,__VA_ARGS__);};
#define DLVPRINTLN(level,txt,...) if (DEBUG_LEVEL > level) {Serial.printf(txt,__VA_ARGS__);Serial.println("");};

#define DLVARPRINT(level,txt,...) if (DEBUG_LEVEL > level) {Serial.print(txt);Serial.print(__VA_ARGS__);Serial.print(":");};
#define DLVARPRINTLN(level,txt,...) if (DEBUG_LEVEL > level) {Serial.print(txt);Serial.print(__VA_ARGS__);Serial.println(":");};
#define DLPRINT(level,...)  if (DEBUG_LEVEL > level) {Serial.print(__VA_ARGS__);};
#define DLPRINTLN(level,...)if (DEBUG_LEVEL > level) {Serial.println(__VA_ARGS__);};
#define DLPRINTINFO(level,...)    \
	   if (DEBUG_LEVEL > level){ \
		Serial.print(millis());     \
	   Serial.print(": ");    \
	   Serial.print(__PRETTY_FUNCTION__); \
	   Serial.print(' ');      \
	   Serial.print(__FILE__);     \
	   Serial.print(':');      \
	   Serial.print(__LINE__);     \
	   Serial.print(' ');      \
	   Serial.println(__VA_ARGS__);};
#else
#define DLVARPRINT(level,txt,...)
#define DLVARPRINTLN(level,txt,...)
#define DLPRINT(level,...)
#define DLPRINTLN(level,...)
#define DLPRINTINFO(level,...)

#define DO_LOGE( format, ... )
#define DO_LOGW( format, ... )
#define DO_LOGI( format, ... )
#define DO_LOGD( format, ... )
#define DO_LOGV( format, ... )

#define ESP_EARLY_LOGE(TAG, format, ...)
#define ESP_EARLY_LOGW(TAG, format, ...)
#define ESP_EARLY_LOGD(TAG, format, ...)
#define ESP_EARLY_LOGV(TAG, format, ...)
#define ESP_EARLY_LOGI(TAG, format, ...)

#define ESP_LOGE(TAG, format, ...)
#define ESP_LOGW(TAG, format, ...)
#define ESP_LOGD(TAG, format, ...)
#define ESP_LOGV(TAG, format, ...)
#define ESP_LOGI(TAG, format, ...)

#define ESP_ERROR_CHECK(func)

#endif

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
