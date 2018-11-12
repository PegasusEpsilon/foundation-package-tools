#include <stdio.h>  	/* puts(), putchar() */
#include <stdlib.h> 	/* exit() */
#include <stdint.h> 	/* uint32_t */
#include <stdarg.h> 	/* va_list, va_start(), vprintf(), va_end() */

__attribute__((cold, noreturn))
void die (const char *restrict const msg) {
	puts(msg);
	exit(1);
}

__attribute__((cold, noreturn))
void fail (const char *restrict const msg) {
	perror(msg);
	exit(1);
}

void debug (uint32_t nested, const char *restrict const fmt, ...) {
	for (uint32_t i = 0; i < nested; i++) putchar('\t');
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}

void assert_fread (void *ptr, size_t size, size_t nmemb, FILE *stream) {
	size_t read = fread(ptr, size, nmemb, stream);
	if (read != size * nmemb) die("Read failed");
}
