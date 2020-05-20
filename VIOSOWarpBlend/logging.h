#ifndef VWB_LOGGING_HPP
#define VWB_LOGGING_HPP

#ifdef VIOSOWARPBLEND_EXPORTS
#define VIOSOWARPBLEND_APIX __declspec(dllexport)
#else
#define VIOSOWARPBLEND_APIX __declspec(dllimport)
#endif

#include "../Include/VWBTypes.h"
VIOSOWARPBLEND_APIX extern VWB_int g_logLevel;
VIOSOWARPBLEND_APIX extern char g_logFilePath[MAX_PATH];

int logStr( VWB_int level, char const* format, ... ); // log someting, ALWAYS returns 0!
void logClear(); // backup and purge logfile

#endif //ndef VWB_LOGGING_HPP
