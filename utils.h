#ifndef UTILS_H
#define UTILS_H

#ifdef _WIN32
#define mkdir(x, y) mkdir(x)
#endif

__attribute__((cold, noreturn))
void die (const char *restrict const msg);
void debug (uint32_t nested, const char *restrict const fmt, ...);

#endif
