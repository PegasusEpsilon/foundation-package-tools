#include <stdio.h>  	/* fopen(), fseek(), ftell(), fclose(), printf() */
#include <stdlib.h> 	/* malloc(), free() */
#include <stdint.h> 	/* uint32_t */
#include <stdbool.h>	/* bool */
#include <stdarg.h> 	/* va_list, va_start(), vprintf(), va_end() */
#include <sys/stat.h>	/* mkdir() */
#include <sys/types.h>	/* mkdir() */

#include "utils.h"  	/* die(), debug(), fread(), chdir() */

void extract_directory (uint32_t nested, FILE *pkg, uint32_t filedata_offset, uint32_t entries) {
	printf("contains %d files/directories\n", entries);
	for (uint32_t filenum = 0; filenum < entries; filenum++) {
		// read the file name
		uint32_t filename_len;
		off_t entry_offset = ftell(pkg);
		assert_fread(&filename_len, sizeof(filename_len), 1, pkg);
		char *filename = malloc(filename_len + 1);
		if (NULL == filename) die("Out of memory allocating filename string storage of all things...");
		assert_fread(filename, filename_len, 1, pkg);
		filename[filename_len] = 0;

		// check if it's a directory
		bool directory; // is it?
		assert_fread(&directory, sizeof(directory), 1, pkg);
		if (directory) { // it is
			uint32_t dir_entries, skip_offset;
			assert_fread(&dir_entries, sizeof(dir_entries), 1, pkg);
			assert_fread(&skip_offset, sizeof(skip_offset), 1, pkg);
			debug(nested, "0x%08X: Directory \"%s\" (skip: 0x%08X) ", entry_offset, filename, skip_offset);
			mkdir(filename, 0777);
			chdir(filename);
			extract_directory(nested + 1, pkg, filedata_offset, dir_entries);
			chdir("..");
			continue;
		}

		// extract the file
		uint32_t offset, length;
		assert_fread(&offset, sizeof(offset), 1, pkg);
		assert_fread(&length, sizeof(length), 1, pkg);
		debug(nested, "0x%08X: File \"%s\" (data: 0x%08X) %d bytes\n", entry_offset, filename, offset + filedata_offset, length);

		off_t header_offset = ftell(pkg);

		fseek(pkg, offset + filedata_offset, SEEK_SET);

		FILE *out = fopen(filename, "wb");
		free(filename); // memory churn don't care

		char *buffer = malloc(length);
		if (NULL == buffer) die("Out of memory allocating copy buffer");
		assert_fread(buffer, length, 1, pkg);
		fwrite(buffer, length, 1, out);
		free(buffer); // memory churn don't care

		fclose(out);

		fseek(pkg, header_offset, SEEK_SET);
	}
}

int main (int argc, char **argv) {
	if (argc != 2) die("need a file to dismantle");
	printf("Package \"%s\" ", argv[1]);
	FILE *pkg = fopen(argv[1], "rb");
	if (NULL == pkg) {
		fflush(stdout);
		fail("failed to open");
	}

	int i;
	for (i = 0; argv[1][i] && argv[1][i] != '.'; i++);
	if (0 == argv[1][i]) die("must have an extension");
	argv[1][i] = 0;

	struct stat statbuf;
	if (0 == stat(argv[1], &statbuf)) {
		printf("cannot be extracted, destination \"%s\"", argv[1]);
		die(" already exists");
	}

	mkdir(argv[1], 0777);
	chdir(argv[1]);

	uint32_t filedata_offset, file_count, total_files;
	assert_fread(&filedata_offset, sizeof(filedata_offset), 1, pkg);
	// the package file format seems to be in a state of flux.
	// if dismantling packages goes completely sideways,
	// comment the next two lines and try again.
	assert_fread(&total_files, sizeof(total_files), 1, pkg);
	printf("(%d files total) ", total_files);
	assert_fread(&file_count, sizeof(filedata_offset), 1, pkg);
	extract_directory(0, pkg, filedata_offset, file_count);
	return 0;
}
