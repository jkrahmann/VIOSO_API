#include "Platform.h"
#include "logging.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
char g_logFilePath[MAX_PATH] = {0};
VWB_int g_logLevel = 2;

int logStr( VWB_int level, char const* format, ... )
{
	if( g_logLevel >= level )
	{
		FILE* f = NULL;
		errno_t err;
		if ( 
			0 == g_logFilePath[0] 
			|| 
			's' == g_logFilePath[0] &&
			't' == g_logFilePath[1] &&
			'd' == g_logFilePath[2] &&
			'o' == g_logFilePath[3] &&
			'u' == g_logFilePath[4] &&
			't' == g_logFilePath[5] &&
			'\0' == g_logFilePath[6]
			)
		{
			f = stdout;
			err = NOERROR;
		}
		else if (
			's' == g_logFilePath[0] &&
			't' == g_logFilePath[1] &&
			'd' == g_logFilePath[2] &&
			'e' == g_logFilePath[3] &&
			'r' == g_logFilePath[4] &&
			'r' == g_logFilePath[5] &&
			'\0' == g_logFilePath[6]
			)
		{
			f = stderr;
		}
		else
		{
			for (int c = 0; c != 10 && 13 == (err = fopen_s(&f, g_logFilePath, "a+")); c++)
			{
				sleep(1);
			}
		}
		if( 0 == err )
		{
            char dest[1024 * 64];
			va_list params;
			
			time_t t;
			time( &t );
			struct tm tm;
			localtime_s( &tm, &t );
			int n = sprintf_s(dest, "%02d:%02d:%02d ", tm.tm_hour, tm.tm_min, tm.tm_sec );
            va_start( params, format );
            vsprintf_s(&dest[n], 1024 * 64 - n, format, params);
            va_end(params);
            
            fputs( dest, f );
#if defined(WIN32) && defined(_DEBUG)
			_CrtDbgReport( _CRT_WARN, NULL, 0, NULL, "%s", dest );
#endif //  defined(WIN32) && defined(_DEBUG)
            fputs("\n", f);
			if( f != stdout && f != stderr )
			{
				fclose(f);
			}
		}
		else
		{
			int i = 0;
		}
	}
	return 0;
}

void logClear()
{
	FILE* f = NULL;
	char szBakFile[MAX_PATH];
	strcpy_s( szBakFile, g_logFilePath );
	strcat_s( szBakFile, ".bak" );
	remove( szBakFile );
	rename( g_logFilePath, szBakFile );

	if( 0 == fopen_s( &f, g_logFilePath, "w+" ) )
		fclose(f);
}
