#ifndef __MANUS_CAMERA_H
#define __MANUS_CAMERA_H

#include <string>
#include <vector>
#include <opencv2/opencv.hpp>

#include "server.h"
#include "threads.h"
#include "pattern.h"

namespace manus {

class CameraHandler : public Handler {
public:

    CameraHandler(int id = 0);

    ~CameraHandler();

    virtual void handle(Request& request);

    bool capture();

private:

    bool localize_camera(vector<PatternDetection> detections    );
    vector<Matx44f> pattern_offsets;

	cv::Mat intrinsics;
	cv::Mat distortion;

    cv::Mat rotation;
    cv::Mat homography;
    cv::Mat translation;

    manus::PatternDetector detector;

    cv::VideoCapture* device;

    cv::Mat frame;

    THREAD_MUTEX camera_mutex;

    THREAD camera_thread;

};

}

#endif

