#ifndef ARCHIVE_FUNCTIONS_H
#define ARCHIVE_FUNCTIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <zlib.h>
#include <tar.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <utime.h>
#include <archive.h>
#include <archive_entry.h>
#include <stdbool.h>

#define PATH_MAX 4096

typedef struct posix_header
{                              /* byte offset */
    char name[100];               /*   0 */
    char mode[8];                 /* 100 */
    char uid[8];                  /* 108 */
    char gid[8];                  /* 116 */
    char size[12];
    char mtime[12];               /* 136 */
    char chksum[8];               /* 148 */
    char typeflag;                /* 156 */
    char linkname[100];           /* 157 */
    char magic[6];                /* 257 */
    char version[2];              /* 263 */
    char uname[32];               /* 265 */
    char gname[32];               /* 297 */
    char devmajor[8];             /* 329 */
    char devminor[8];             /* 337 */
    char prefix[155];             /* 345 */
    /* 500 */
} posix_header;

int next_file_in_tar(gzFile* archive, posix_header* header_out);
void view_arcive_contents(gzFile* archive);
void view_archive_contents_with_dates(const char *archive_path);
void print_checksum(posix_header* header);
void copy_archive(gzFile* src, FILE* dst, posix_header* header);
void archivate(FILE* file, const char* path);
void display_menu(const char *archive_path);
void process_selected_archive(const char *archive_path);

#endif 
