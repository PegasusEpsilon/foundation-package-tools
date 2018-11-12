#include <stdio.h>
#include <stdlib.h> 	/* exit() */
#include <stdint.h> 	/* uint32_t */
#include <stdarg.h> 	/* va_list, va_start(), vprintf(), va_end() */
#include <sys/types.h>	/* struct stat, stat() */
#include <sys/stat.h>	/* struct stat, stat() */
#include <unistd.h> 	/* struct stat, stat(), chdir() */

int main (int argc, char **argv) {
	if (argc != 2) die("need a directory to package");
	return 0;
}
