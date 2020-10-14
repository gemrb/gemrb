#ifndef __OS4HELPERS_H__
#define __OS4HELPERS_H__

#undef __STRICT_ANSI__

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <string>
#include <sstream>

#include <errno.h>
#include <fnmatch.h>

#include <math.h>

#define UNUSED(x) (void)(x)

#undef HAVE_MEMALIGN

namespace std { 
    template <typename T> string to_string(const T &n) { 
        ostringstream strm; 
        strm << n; 
        return strm.str(); 
    } 

    typedef basic_string<wchar_t> wstring; 
    typedef basic_ostream<wchar_t> wostream;
    typedef basic_ostringstream<wchar_t> wostringstream;
    typedef basic_stringstream<wchar_t> wstringstream;

    int iswspace(wint_t c);

    long int lround(double x);
}

typedef struct {
	wchar_t *buffer;
	size_t space;
} _putwc_data;

typedef struct {
	FILE *fp;
	mbstate_t mbs;
} _putwc_data_cb;

#define PROT_NONE 0x0
#define PROT_READ 0x1
#define PROT_WRITE 0x2
#define PROT_EXEC 0x4

/* git__mmmap() flags values */
#define MAP_FILE	0
#define MAP_SHARED 1
#define MAP_PRIVATE 2
#define MAP_TYPE	0xf
#define MAP_FIXED	0x10
#define MAP_FAILED 0

typedef int (*libwprintf_putwc_cb)(wchar_t c, void *udata);

int _libwprintf_dofmt(libwprintf_putwc_cb cb, void *cb_data, const wchar_t *fmt, va_list ap);


#ifdef __cplusplus
extern "C" {
#endif

int fnmatch (const char *pattern, const char *string, int flags);

int swprintf(wchar_t *s, size_t n, const wchar_t *fmt, ...);
int vswprintf(wchar_t *s, size_t n, const wchar_t *fmt, va_list ap);

int fwprintf(FILE *fp, const wchar_t *fmt, ...);
int vfwprintf(FILE *fp, const wchar_t *fmt, va_list ap);
void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset);
int munmap(void *map, size_t length);

#ifdef __cplusplus
}
#endif

#undef __STRICT_ANSI__

#endif

