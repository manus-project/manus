
#ifndef __FILESYSTEM_H
#define __FILESYSTEM_H

#define FILETYPE_NONE 0
#define FILETYPE_FILE 1
#define FILETYPE_DIRECTORY 2
#define FILETYPE_OTHER 100

#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#define FILE_DELIMITER '\\'
#define FILE_DELIMITER_STR "\\"
#else
#define FILE_DELIMITER '/'
#define FILE_DELIMITER_STR "/"
#endif

int file_read(const char* filename, char** buffer);

int file_type(const char* filename);

char* path_join(const char* root, const char* path);

char* path_parent(const char* path);

#ifdef __cplusplus

#include <string>

using namespace std;


int file_read(const string& filename, char** buffer);

int file_type(const string& filename);

string path_join(const string& root, const string& path);

string path_parent(const string& path);

#endif

#endif
