
#include "utilities.h"
#include "filesystem.h"
#include "debug.h"

#define embedded_header_only
#include "embedded.c"

Point3f extractHomogeneous(Matx41f hv)
{
    Point3f f = Point3f(hv(0, 0) / hv(3, 0), hv(1, 0) / hv(3, 0), hv(2, 0) / hv(3, 0));
    return f;
}

bool get_resource(const string& file, char** buffer, size_t* length) {

#ifdef SERVER_DOCUMENT_ROOT

	string path = path_join(SERVER_DOCUMENT_ROOT, file);

	if (file_type(path) == FILETYPE_FILE) {
        *length = file_read(path, buffer);
		DEBUGMSG("Found local copy: %s\n", path.c_str())
		return true;
    } else {
        return false;
    }

#else
	return embedded_copy_resource(file.c_str(), buffer, length);
#endif

}


