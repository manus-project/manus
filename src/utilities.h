
#ifndef __MANUS_UTILITIES_H
#define __MANUS_UTILITIES_H

#include <opencv2/core/core.hpp>

using namespace std;
using namespace cv;

Point3f extractHomogeneous(Matx41f hv);

bool get_resource(const string& file, char** buffer, size_t* length);

#endif

