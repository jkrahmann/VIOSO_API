//
// Created by sven on 6/13/20.
//

#include "linux_compat.h"

errno_t strcat_s(char* dest, size_t n, const char* src) { return n <= strlcat(dest, src, n); }
errno_t strcpy_s(char* dest, size_t n, const char* src) { return n <= strlcpy(dest, src, n); }
errno_t strncpy_s(char* dest, size_t n, const char* src, size_t count) { if (n < count) count = n; return n <= strlcpy(dest, src, count); }
