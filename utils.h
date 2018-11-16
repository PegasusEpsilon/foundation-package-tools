#ifndef UTILS_H
#define UTILS_H

#if (defined(_WIN32) || defined(__WIN32__))
#include <direct.h>
#define mkdir(x, y) _mkdir(x)
#endif

void assert_chdir(char *dir);
#define chdir(x) assert_chdir(x)

void assert_fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
//#define fread(a, b, c, d) assert_fread(a, b, c, d)

__attribute__((cold, noreturn))
void die (const char *restrict const msg);
__attribute__((cold, noreturn))
void fail (const char *restrict const msg);
void debug (uint32_t nested, const char *restrict const fmt, ...);

#endif
