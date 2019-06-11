
#include <iostream>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <fstream>
#include <vector>

#include "files.h"

void tokenize(std::string const &str, const char delim,
			std::vector<std::string> &out)
{
	size_t start;
	size_t end = 0;

	while ((start = str.find_first_not_of(delim, end)) != std::string::npos)
	{
		end = str.find(delim, start);
		out.push_back(str.substr(start, end - start));
	}
}

vector<string> search_paths;

inline bool file_exists(const string& name) {
    ifstream f(name.c_str());
    return f.good();
}

inline string path_join(const string& base, const string path) {

	if (base[base.size()-1] == '/') {
		return base + path;
	} else {
		return base + string("/") + path;
	}

}

string get_env(const string &name, const string& def) {

	if (getenv(name.c_str())) {
		return string(getenv(name.c_str()));
	} else {
		return def;
	}

}

string find_file(const string &name) {

	if (search_paths.size() == 0) {

		if (getenv("MANUS_PATH")) {

			tokenize(string(getenv("MANUS_PATH")), ':', search_paths);

		}

		prepend_path("./");

	}

	for (vector<string>::iterator it = search_paths.begin(); it != search_paths.end(); it++) {

		string candidate = path_join(*it, name);

		if (file_exists(candidate)) return candidate;
	}

	return name;
}

void append_path(const string &path) {

	if (path.size() < 1) return;

	search_paths.push_back(path);

}


void prepend_path(const string &path) {

	if (path.size() < 1) return;

	search_paths.insert(search_paths.begin(), path);

}
