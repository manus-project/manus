
#ifndef __MARKERS_H
#define __MARKERS_H

#include <string>
#include <vector>
#include <map>
#include <opencv2/opencv.hpp>
#include "server.h"

using namespace std;
using namespace cv;

namespace manus {

typedef struct MarkerInfo {
    string name;
	Point3f position;
	float size;
	Point3f orientation;
	Scalar color;
} ArmInfo;

class MarkersApiHandler : public Handler {
public:
    MarkersApiHandler();
    ~MarkersApiHandler();

    virtual void handle(Request& request);

private:

	int counter = 0;

	map<int, MarkerInfo> markers;

};

}

#endif

