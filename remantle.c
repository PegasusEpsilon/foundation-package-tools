#define _POSIX_C_SOURCE 200112L
#define _XOPEN_SOURCE 500
#include <stdio.h>  	/* FILE, printf() */
#include <stdlib.h> 	/* mkstemp() */
#include <stdint.h> 	/* uint32_t */
#include <stdbool.h>	/* bool, true, false */
#include <stdarg.h> 	/* va_list, va_start(), vprintf(), va_end() */
#include <sys/types.h>	/* struct stat, stat(), opendir() */
#include <sys/stat.h>	/* struct stat, stat() */
#include <dirent.h> 	/* struct dirent, readdir() */
#include <unistd.h> 	/* struct stat, stat(), */
#include <libgen.h> 	/* basename() */
#include <string.h> 	/* strlen(), strcat(), strcpy() */

#include "utils.h"  	/* die(), debug(), fread(), chdir() */

#define BLOCK 8192

uint32_t pack (FILE *idx, FILE *dat, uint32_t nested) {
	struct stat statbuf;
	uint32_t file_count = 0, entry_count = 0, subdir_file_count = 0;
	DIR *dir = opendir(".");

	// every directory entry in the newer format, including root directory
	// starts with file count, so, reserve uint32_t for file count
	// and fill it in at the end
	long file_count_offset = ftell(idx);
	fseek(idx, sizeof(uint32_t), SEEK_CUR);

	// every directory except the root directory has a skip offset immediately
	// after the file count. swapping these two values would make this much
	// simpler...

	if (nested)
		fseek(idx, sizeof(uint32_t), SEEK_CUR);

	for (;;) {
		struct dirent *entry = readdir(dir);
		if (NULL == entry) break;
		if ('.' == *entry->d_name) continue;
		FILE *src = fopen(entry->d_name, "rb");
		if (fstat(fileno(src), &statbuf))
			fail("failed to stat() open filehandle");
		entry_count++;
		size_t len = strlen(entry->d_name);
		fwrite(&len, sizeof(len), 1, idx);
		fwrite(entry->d_name, len, 1, idx);
		if (S_ISDIR(statbuf.st_mode)) {
			debug(nested, "Directory: \"%s\"\n", entry->d_name);
			fclose(src);
			chdir(entry->d_name);
			fwrite("\1", sizeof(bool), 1, idx);
			subdir_file_count += pack(idx, dat, nested + 1);
			chdir("..");
			continue;
		}
		fwrite("\0", sizeof(bool), 1, idx);
		long data_offset = ftell(dat);
		fwrite((uint32_t *)&data_offset, sizeof(uint32_t), 1, idx);
		fwrite((uint32_t *)&statbuf.st_size, sizeof(uint32_t), 1, idx);
		size_t readbytes;
		uint8_t buf[BLOCK];
		do {
			readbytes = fread(buf, 1, BLOCK, src);
			fwrite(buf, 1, readbytes, dat);
		} while (BLOCK == readbytes);
		fclose(src);
		debug(nested, "File: \"%s\" offset 0x%lX size %ld\n", entry->d_name, data_offset, statbuf.st_size);
		file_count++;
	}
	// fill in file count that we reserved earlier
	off_t entry_end_offset = ftell(idx);
	fseek(idx, file_count_offset, SEEK_SET);
	fwrite((uint32_t *)&entry_count, sizeof(uint32_t), 1, idx);
	// fill in the directory entry skip offset, too, if we're in a subdirectory
	if (nested)
		fwrite((uint32_t *)&entry_end_offset, sizeof(uint32_t), 1, idx);
	fseek(idx, entry_end_offset, SEEK_SET);
	return file_count + subdir_file_count;
}

int main (int argc, char **argv) {
	struct stat statbuf;
	if (argc != 2) die("need a directory to package");
	argv[1] = basename(argv[1]);
	char *filename = strcat(strcat(malloc(strlen(argv[1]) + strlen(".package") + 1), argv[1]), ".package");
	if (!stat(filename, &statbuf))
		die("refusing to overwrite extant package file, back it up, and get it out of the way first!");
	if (stat(argv[1], &statbuf))
		fail("failed to stat() source directory");
	if (!S_ISDIR(statbuf.st_mode))
		die("refusing to remantle a non-directory");

	printf("Creating package %s from directory %s...\n", filename, argv[1]);
	FILE *pkg = fopen(filename, "wb");

	// mangle the filename into a temp file name
	size_t len = strlen(filename);
	// this replaces "x.package", created above, with "x.XXXXXX" -- always safe
	strcpy(&filename[len-7], "XXXXXX");
	// so we can make a temp file out of it
	int tmpfd = mkstemp(filename);
	unlink(filename); // early unlink, our buffer is now a ghost
	FILE *tmp = fdopen(tmpfd, "wb");
	// since we don't need it later anyway
	free(filename);

	chdir(argv[1]);
	// set up header fields
	// uint32_t file data offset (must index first)
	// uint32_t total file count (must index first)
	// so our first entry offset is +4 +4 = 8
	// and we skip fields to be populated later
	fseek(pkg, 8, SEEK_SET);
	uint32_t total_files = pack(pkg, tmp, 0);
	long data_offset = ftell(pkg);

	// copy temp file into the package
	fseek(tmp, 0, SEEK_SET);
	uint8_t buf[BLOCK];
	size_t readbytes;
	do {
		readbytes = fread(buf, 1, BLOCK, tmp);
		fwrite(buf, 1, readbytes, pkg);
	} while (BLOCK == readbytes);
	fclose(tmp);
	fseek(pkg, 0, SEEK_SET);
	fwrite((uint32_t *)&data_offset, sizeof(uint32_t), 1, pkg);
	fwrite(&total_files, sizeof(total_files), 1, pkg);
	fclose(pkg);
	return 0;
}
