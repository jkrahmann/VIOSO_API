//
//  mac_compat.h
//  VIOSOWarpBlend
//
//  Created by Tom Riley on 5/19/17.
//

#ifndef LINUX_COMPAT_H
#define LINUX_COMPAT_H

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <bsd/string.h>
#include <string>

typedef int errno_t;
static const errno_t NO_ERROR = 0;

errno_t strcat_s(char* dest, size_t n, const char* src); // { return n <= strlcat(dest, src, n); }
errno_t strcpy_s(char* dest, size_t n, const char* src); // { return n <= strlcpy(dest, src, n); }
errno_t strncpy_s(char* dest, size_t n, const char* src, size_t count); // { if (n < count) count = n; return n <= strlcpy(dest, src, count); }

#define vfprintf_s vfprintf
#define fread_s(a,b,c,d,e) fread(a,c,d,e)
#define _stricmp strcasecmp
#define sscanf_s sscanf
#define sprintf_s sprintf
#define sleep(ms) usleep(ms*1000)
#define InterlockedIncrement(ptr) __sync_fetch_and_add(ptr, 1)
#define InterlockedDecrement(ptr) __sync_fetch_and_sub(ptr, 1)

inline int fopen_s( FILE** f, const char * __restrict __filename, const char * __restrict __mode) {
    *f = fopen(__filename, __mode);
    return (*f ? 0 : -1);
}

#endif // LINUX_COMPAT_H
