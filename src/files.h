


#ifndef __MANUS_FILES
#define __MANUS_FILES

#include <string>

using namespace std;

string get_env(const string &name, const string& def = string(""));

string find_file(const string &name);

void append_path(const string &path);

void prepend_path(const string &path);


#endif