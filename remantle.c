#include <stdio.h>  	/* FILE, printf() */
#include <stdlib.h> 	/* exit() */
#include <stdint.h> 	/* uint32_t */
#include <stdbool.h>	/* bool, true, false */
#include <stdarg.h> 	/* va_list, va_start(), vprintf(), va_end() */
#include <sys/types.h>	/* struct stat, stat(), opendir() */
#include <sys/stat.h>	/* struct stat, stat() */
#include <dirent.h> 	/* struct dirent, readdir() */
#include <unistd.h> 	/* struct stat, stat(), */
#include <libgen.h> 	/* basename() */
#include <string.h> 	/* strlen(), strcat() */

#include "utils.h"  	/* die(), debug(), fread(), chdir() */

uint32_t build_index (FILE *pkg, off_t data_offset) {
	struct stat statbuf;
	uint32_t file_count = 0, entry_count, subdir_file_count = 0;
	DIR *dir = opendir(".");

	// every directory entry in the newer format, including root directory
	// starts with file count, so, reserve uint32_t for file count
	// and fill it in at the end
	int file_count_offset = ftell(pkg);
	fseek(pkg, sizeof(uint32_t), SEEK_CUR);

	// every directory except the root directory has a skip offset immediately
	// after the file count. swapping these two values would make this much
	// simpler...

	int subdir = data_offset;
	if (subdir)
		fseek(pkg, sizeof(uint32_t), SEEK_CUR);

	for (;;) {
		struct dirent *entry = readdir(dir);
		if (NULL == entry) break;
		if ('.' == *entry->d_name) continue;
		if (stat(entry->d_name, &statbuf))
			fail("failed to stat() directory entry");
		entry_count++;
		uint32_t len = strlen(entry->d_name);
		fwrite(&len, sizeof(len), 1, pkg);
		fwrite(entry->d_name, len, 1, pkg);
		if (S_ISDIR(statbuf.st_mode)) {
			chdir(entry->d_name);
			fwrite("\1", sizeof(bool), 1, pkg);
			subdir_file_count += build_index(pkg, data_offset);
			chdir("..");
			continue;
		}
		fwrite("\0", sizeof(bool), 1, pkg);
		fwrite((uint32_t *)&data_offset, sizeof(uint32_t), 1, pkg);
		fwrite((uint32_t *)&statbuf.st_size, sizeof(uint32_t), 1, pkg);
//		printf("File: \"%s\" offset 0x%lX size %ld\n", entry->d_name, data_offset, statbuf.st_size);
		data_offset += statbuf.st_size;
		file_count++;
	}
	// fill in file count that we reserved earlier
	off_t entry_end_offset = ftell(pkg);
	fseek(pkg, file_count_offset, SEEK_SET);
	fwrite((uint32_t *)&entry_count, sizeof(uint32_t), 1, pkg);
	// fill in the directory entry skip offset, too, if we're in a subdirectory
	if (subdir) fwrite((uint32_t *)&entry_end_offset, sizeof(uint32_t), 1, pkg);
	fseek(pkg, entry_end_offset, SEEK_SET);
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
	FILE *pkg = fopen(filename, "w");
	free(filename);
	chdir(argv[1]);
	// set up header fields
	// uint32_t file data offset (must index first)
	// uint32_t total file count (must index first)
	// so our first entry offset is +4 +4 = 8
	// and we skip fields to be populated later
	fseek(pkg, 8, SEEK_SET);
	uint32_t total_files = build_index(pkg, 0);
	off_t data_offset = ftell(pkg);
	fseek(pkg, 0, SEEK_SET);
	fwrite((uint32_t *)&data_offset, sizeof(uint32_t), 1, pkg);
	fwrite(&total_files, sizeof(total_files), 1, pkg);
	fclose(pkg);
	return 0;
}
