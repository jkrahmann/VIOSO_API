#ifndef VWB_LOGGING_HPP
#define VWB_LOGGING_HPP

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#ifdef VIOSOWARPBLEND_EXPORTS
#define VIOSOWARPBLEND_APIX __declspec(dllexport)
#else
#define VIOSOWARPBLEND_APIX __declspec(dllimport)
#endif
#endif

#include "../Include/VWBTypes.h"
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
VIOSOWARPBLEND_APIX extern VWB_int g_logLevel;
VIOSOWARPBLEND_APIX extern char g_logFilePath[MAX_PATH];
#else
extern VWB_int g_logLevel;
extern char g_logFilePath[MAX_PATH];
#endif

int logStr( VWB_int level, char const* format, ... ); // log someting, ALWAYS returns 0!
void logClear(); // backup and purge logfile

#endif //ndef VWB_LOGGING_HPP
