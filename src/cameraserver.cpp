#include <unistd.h>

#include <iostream>
#include <fstream>
#include <memory>
#include <chrono>
#include <opencv2/opencv.hpp>

#include <echolib/client.h>
#include <echolib/datatypes.h>
#include <echolib/helpers.h>
#include <echolib/loop.h>

#include <echolib/opencv.h>

#include "files.h"

using namespace std;
using namespace echolib;
using namespace cv;

CameraIntrinsics parameters;

#define READ_MATX(N, M) { Mat tmp; (N) >> tmp; (M) = tmp; }

int main(int argc, char** argv) {

    int cameraid = (argc < 2 ? 0 : atoi(argv[1]));
    double fps = 30;
    bool initialized = true;

    SharedClient client = echolib::connect(string(), "cameraserver");

    VideoCapture device;
    Mat image;

    string calibration_path = find_file("camera_calibration.xml");
    string config_path = find_file("camera.yaml");

    device.open(cameraid);

    if (!device.isOpened()) {
        cerr << "Cannot open camera device " << cameraid << endl;
        initialized = false;
    }

    FileStorage fsc(config_path, FileStorage::READ);
    if (fsc.isOpened()) {
        if (!fsc["limit_fps"].empty()) {
            fsc["limit_fps"] >> fps; 
            fps = min(1000.0, max(0.1, fps));
        } 
    }

    Size calib_size(640, 480);
    FileStorage fscalib(calibration_path, FileStorage::READ);
    if (fscalib.isOpened()) {
        
        if (!fscalib["intrinsic"].empty()) {
            READ_MATX(fscalib["intrinsic"], parameters.intrinsics);
            fscalib["distortion"] >> parameters.distortion;

        } else if (!fscalib["cameraMatrix"].empty()) {
            READ_MATX(fscalib["cameraMatrix"], parameters.intrinsics);
            fscalib["dist_coeffs"] >> parameters.distortion;
            fscalib["cameraResolution"] >> calib_size;
        }

        if (initialized) {
            device.set(CV_CAP_PROP_FRAME_WIDTH, calib_size.width);
            device.set(CV_CAP_PROP_FRAME_HEIGHT, calib_size.height);
        
            device >> image;
        }

    } else {
        if (initialized) {
            device >> image;

            parameters.intrinsics(0, 0) = 700;
            parameters.intrinsics(1, 1) = 700;
            parameters.intrinsics(0, 2) = (float)(image.cols) / 2;
            parameters.intrinsics(1, 2) = (float)(image.rows) / 2;
            parameters.intrinsics(2, 2) = 1;
            parameters.distortion = (Mat_<float>(1,5) << 0, 0, 0, 0, 0);

            calib_size = Size(image.cols, image.rows);
        }
    }

    parameters.width = calib_size.width;
    parameters.height = calib_size.height;

    SharedTypedPublisher<Frame> frame_publisher = make_shared<TypedPublisher<Frame> >(client, "camera", 1);

    SubscriptionWatcher watcher(client, "camera");

    StaticPublisher<CameraIntrinsics> intrinsics_publisher = StaticPublisher<CameraIntrinsics>(client, "intrinsics", parameters);

    std::chrono::system_clock::time_point a = std::chrono::system_clock::now();
    std::chrono::system_clock::time_point b = std::chrono::system_clock::now();

    if (!initialized) {
        image.create(calib_size, CV_8UC3);
        image.setTo(255);
        line(image, Point(0, 0), Point(image.cols, image.rows), Scalar(0, 0, 0), 2);
        line(image, Point(image.cols, 0), Point(0, image.rows), Scalar(0, 0, 0), 2);
    }

    while (true) {

        if (initialized) device >> image;

        a = std::chrono::system_clock::now();
        std::chrono::duration<double, std::milli> work_time = a - b;

        b = a;

        if (image.empty()) return -1;

        if (watcher.get_subscribers() > 0) {
            Frame frame(Header("camera", a), image);

            frame_publisher->send(frame);
        }

        std::chrono::duration<double, std::milli> delta_ms(max(0.0, 1000.0 / fps - (double)work_time.count()));
        
        auto delta_ms_duration = std::chrono::duration_cast<std::chrono::milliseconds>(delta_ms);

        if (!echolib::wait(max(10, (int)delta_ms_duration.count()))) break;
    }

    exit(0);
}
