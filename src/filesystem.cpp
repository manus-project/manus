
#include <fstream>
#include <stdio.h>
#include <string.h>
#include <sstream>
#include <exception>
#include <string>

#include "filesystem.h"

using namespace std;

static char *sdup(const char *str) {
  char *p;
  if ((p = (char *) malloc(strlen(str) + 1)) != NULL) {
    strcpy(p, str);
  }
  return p;
}

int file_read(const char* filename, char** buffer) {

	FILE *fp = fopen(filename, "r");

	if (fp != NULL) {
	    /* Go to the end of the file. */
	    if (fseek(fp, 0L, SEEK_END) == 0) {
	        /* Get the size of the file. */
	        long bufsize = ftell(fp);
	        if (bufsize == -1) { /* Error */ }

	        /* Allocate our buffer to that size. */
	        *buffer = (char*) malloc(sizeof(char) * (bufsize));

	        /* Go back to the start of the file. */
	        if (fseek(fp, 0L, SEEK_SET) == 0) { /* Error */ }

	        /* Read the entire file into memory. */
	        size_t newLen = fread(*buffer, sizeof(char), bufsize, fp);
	        if (newLen == 0) {
	        	free(*buffer);
	        	return -1;
	        } 

            return bufsize;
	    }
	    fclose(fp);
	}

    return -1;
}

int file_read(const string& filename, char** buffer) {
    return file_read(filename.c_str(), buffer);
}


#ifdef _WIN32

int file_type(const char* filename) {

#pragma message ("Warning : must implement")
	 return FILETYPE_FILE ;

}

string path_join(const string& root, const string& path) {

	if (root[root.size()-1] == '\\')
        return root + path;
    else if (path[0] == '\\')
        return path;
    else return root + "\\" + path;

}

string path_parent(const string& path) {

    if (path.size() < 2)
        return path;

    int loc = path.rfind('\\', (path[path.size()-1] == '\\') ? path.size()-2 : string::npos);

    if (loc != string::npos) {
        return path;
    } else {
        return path.substr(0, loc);
    }

}

#else

#include <fstream>
#include <errno.h>
#include <sys/stat.h>

int file_type(const char* filename) {

    struct stat buf;
    if (stat (filename, &buf) == -1 && errno == ENOENT)
        return FILETYPE_NONE;

    if (S_ISREG (buf.st_mode)) {
        return FILETYPE_FILE;
    }
    if (S_ISDIR (buf.st_mode)) {
        return FILETYPE_DIRECTORY;
    }

    return FILETYPE_OTHER;

}

string path_join(const string& root, const string& path) {

    if (path[0] == '/')
        return path;
    else if (root[root.size()-1] == '/')
        return root + path; 
    else return root + "/" + path;

}

string path_parent(const string& path) {

    if (path.size() < 2)
        return path;

    int loc = path.rfind('/', (path[path.size()-1] == '/') ? path.size()-2 : string::npos);

    if (loc == string::npos) {
        return path;
    } else {
        return path.substr(0, loc);
    }

}

#endif

int file_type(const string& filename) {

    return file_type(filename.c_str());

}

char* path_join(const char* root, const char* path) {

    string result = path_join(string(root), string(path));

    return sdup(result.c_str());

}

char* path_parent(const char* path) {

    string result = path_parent(string(path));

    return sdup(result.c_str());

}


