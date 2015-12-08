

#include "camera.h"
#include "definitions.h"
#include "json.h"

#define embedded_header_only
#include "embedded.c"

#include "debug.h"

using namespace cv;

namespace manus {

Matx44f rotateMatrix(float x, float y, float z)
{
    Matx44f RX = Matx44f(
              1, 0,      0,       0,
              0, cos(x), -sin(x), 0,
              0, sin(x),  cos(x), 0,
              0, 0,       0,     1);

    Matx44f RY = Matx44f(
              cos(y), 0, -sin(y), 0,
              0, 1,          0, 0,
              sin(y), 0,  cos(y), 0,
              0, 0,          0, 1);

    Matx44f RZ = Matx44f(
              cos(z), -sin(z), 0, 0,
              sin(z),  cos(z), 0, 0,
              0,          0,           1, 0,
              0,          0,           0, 1);

    return RX * RY * RZ;

}

Matx44f translateMatrix(float x, float y, float z)
{
    return Matx44f(
              1, 0, 0, x,
              0, 1, 0, y,
              0, 0, 1, z,
              0, 0, 0, 1);

}

THREAD_CALLBACK(camera_callback_function, handler) {

    CameraHandler* camera_handler = (CameraHandler*) handler;

    while (true) {
        if (!camera_handler->capture()) break;
        sleep(100);
    }

	return NULL;
}

CameraHandler::CameraHandler(int id) : device(NULL), frame_counter(0) {

    Matx44f rotate = translateMatrix(-25, -25, 0) * rotateMatrix(0, 0, (float) M_PI) * translateMatrix(25, 25, 0);

    detector.loadPattern("marker1.png", 50);
    pattern_offsets.push_back(rotate * translateMatrix(-130, -190, 0));
    detector.loadPattern("marker2.png", 50);
    pattern_offsets.push_back(rotate * translateMatrix(-130, 40, 0));
    
    char* buffer;
    size_t size;

    if (!embedded_copy_resource("calibration.xml", &buffer, &size))
        throw runtime_error("Unable to load calibration data");

    FileStorage fs(string(buffer, size), FileStorage::READ + FileStorage::MEMORY);
    fs["intrinsic"] >> intrinsics;
    fs["distortion"] >> distortion;
    free(buffer);

    device = new VideoCapture(id);

    if (!device->isOpened()) 
        throw runtime_error("Camera not available");


    rotation = Mat::eye(3, 3, CV_32F);
    homography = Mat::eye(3, 3, CV_32F);
    translation = (Mat_<float>(3,1) << 0, 0, 0);

    MUTEX_INIT(camera_mutex);

    CREATE_THREAD(camera_thread, camera_callback_function, this);

}

CameraHandler::~CameraHandler() {

    MUTEX_LOCK(camera_mutex);

    if (device) {
        delete device;
        device = NULL;
    }

    MUTEX_UNLOCK(camera_mutex);

    MUTEX_DESTROY(camera_mutex);

    RELEASE_THREAD(camera_thread);

}

void CameraHandler::handle(Request& request) {


    if (!request.has_variable("command")) {
        request.set_status(404); 
        request.send_data("Not found");
        request.finish();
        return;
    } 

    string command = request.get_variable("command");

    if (command == "image") {

        MUTEX_LOCK(camera_mutex);

        Mat temp = frame;

        MUTEX_UNLOCK(camera_mutex);

        if (!temp.empty()) {

            vector<int> params;
            params.push_back(IMWRITE_JPEG_QUALITY);
            params.push_back(80);

            vector<uchar> buffer;
            if (imencode(".jpg", temp, buffer, params)) {
                request.set_status(200); 
                request.set_header("Content-Type", "image/jpeg");
                request.set_header("Cache-Control", "max-age=0, post-check=0, pre-check=0, no-store, no-cache, must-revalidate");
                request.send_data(&(buffer[0]), (int) buffer.size());
            } else {
                request.set_status(404); 
                request.send_data("Not found");
            }

        } else {
            request.set_status(404); 
            request.send_data("Not found");
        }

    } /*else if (command == "save") {

        MUTEX_LOCK(camera_mutex);

        Mat temp = frame;

        MUTEX_UNLOCK(camera_mutex);

        imwrite("/tmp/test.png", temp);

        cout << rotation << endl;
        cout << translation << endl;

        request.set_status(200); 
        request.send_data("OK");

    }*/ else if (command == "describe") {

        MUTEX_LOCK(camera_mutex);

        Mat temp = frame;

        MUTEX_UNLOCK(camera_mutex);

        Json::Value response;

        Json::Value intr;
        intr["m00"] = Json::Value(intrinsics.at<float>(0, 0));
        intr["m10"] = Json::Value(intrinsics.at<float>(1, 0));
        intr["m20"] = Json::Value(intrinsics.at<float>(2, 0));
        intr["m01"] = Json::Value(intrinsics.at<float>(0, 1));
        intr["m11"] = Json::Value(intrinsics.at<float>(1, 1));
        intr["m21"] = Json::Value(intrinsics.at<float>(2, 1));
        intr["m02"] = Json::Value(intrinsics.at<float>(0, 2));
        intr["m12"] = Json::Value(intrinsics.at<float>(1, 2));
        intr["m22"] = Json::Value(intrinsics.at<float>(2, 2));

        Json::Value dist;
        dist["m0"] = Json::Value(distortion.at<float>(0, 0));
        dist["m1"] = Json::Value(distortion.at<float>(0, 1));
        dist["m2"] = Json::Value(distortion.at<float>(0, 2));
        dist["m3"] = Json::Value(distortion.at<float>(0, 3));

        response["intrinsics"] = intr;
        response["distortion"] = dist;

        Json::Value image;
        image["width"] = Json::Value(temp.cols); // Reference sensor size (typical for most webcams)
        image["height"] = Json::Value(temp.rows);
        response["image"] = image;

//calibrationMatrixValues(intrinsics, temp.size(), double apertureWidth, double apertureHeight, double& fovx, double& fovy, double& focalLength, Point2d& principalPoint, double& aspectRatio)

        request.set_status(200); 
        request.set_header("Content-Type", "application/json");
        request.set_header("Cache-Control", "max-age=0, post-check=0, pre-check=0, no-store, no-cache, must-revalidate");

        Json::FastWriter writer;
        request.send_data(writer.write(response));


    } else if (command == "position") {

        Json::Value response;

        Json::Value rot;
        rot["m00"] = Json::Value(rotation.at<float>(0, 0));
        rot["m10"] = Json::Value(rotation.at<float>(1, 0));
        rot["m20"] = Json::Value(rotation.at<float>(2, 0));
        rot["m01"] = Json::Value(rotation.at<float>(0, 1));
        rot["m11"] = Json::Value(rotation.at<float>(1, 1));
        rot["m21"] = Json::Value(rotation.at<float>(2, 1));
        rot["m02"] = Json::Value(rotation.at<float>(0, 2));
        rot["m12"] = Json::Value(rotation.at<float>(1, 2));
        rot["m22"] = Json::Value(rotation.at<float>(2, 2));

        Json::Value trans;
        trans["m0"] = Json::Value(translation.at<float>(0, 0));
        trans["m1"] = Json::Value(translation.at<float>(0, 1));
        trans["m2"] = Json::Value(translation.at<float>(0, 2));

        Json::Value hom;
        hom["m00"] = Json::Value(homography.at<float>(0, 0));
        hom["m10"] = Json::Value(homography.at<float>(1, 0));
        hom["m20"] = Json::Value(homography.at<float>(2, 0));
        hom["m01"] = Json::Value(homography.at<float>(0, 1));
        hom["m11"] = Json::Value(homography.at<float>(1, 1));
        hom["m21"] = Json::Value(homography.at<float>(2, 1));
        hom["m02"] = Json::Value(homography.at<float>(0, 2));
        hom["m12"] = Json::Value(homography.at<float>(1, 2));
        hom["m22"] = Json::Value(homography.at<float>(2, 2));

        response["rotation"] = rot;
        response["translation"] = trans;
        response["homography"] = hom;

        request.set_status(200); 
        request.set_header("Content-Type", "application/json");
        request.set_header("Cache-Control", "max-age=0, post-check=0, pre-check=0, no-store, no-cache, must-revalidate");

        Json::FastWriter writer;
        request.send_data(writer.write(response));


    } else {
        request.set_status(404); 
        request.send_data("Not found");
    }

    request.finish();
}

bool CameraHandler::capture() {

    MUTEX_LOCK(camera_mutex);

    if (!device) return false;

    Mat temp;
    *device >> temp; 
    frame_counter++;

    if (temp.empty()) {
        if (frame_counter % 100 == 0) {
            DEBUGMSG("Unable to retrieve image.");
        }
        return false;
    }

    MUTEX_UNLOCK(camera_mutex);

    if (frame_counter % 10 == 0) {
        vector<PatternDetection> detectedPatterns;
        detector.detect(temp, intrinsics, distortion, detectedPatterns); 

	    /*for (unsigned int i =0; i<detectedPatterns.size(); i++) {
		    detectedPatterns.at(i).draw(temp, intrinsics, distortion);
	    }*/

        MUTEX_LOCK(camera_mutex);

        if (detectedPatterns.size() > 0) {

            localize_camera(detectedPatterns);

        }

        MUTEX_UNLOCK(camera_mutex);

    }

    MUTEX_LOCK(camera_mutex);
    temp.copyTo(frame);
    MUTEX_UNLOCK(camera_mutex);

    return true;
}

inline Point3f extractHomogeneous(Matx41f hv)
{
    Point3f f = Point3f(hv(0, 0) / hv(3, 0), hv(1, 0) / hv(3, 0), hv(2, 0) / hv(3, 0));
    return f;
}

bool CameraHandler::localize_camera(vector<PatternDetection> detections)
{

    if (detections.size() < 1) return false;

    PatternDetection* anchor = NULL;

    Mat rotVec;

    vector<Point3f> surfacePoints;
    vector<Point2f> imagePoints;

    for (unsigned int i =0; i < detections.size(); i++) {

        int id = detections.at(i).getIdentifier();

        if (id >= pattern_offsets.size())
            continue;

        float size = (float) detections.at(i).getSize();

        Matx44f transform =  pattern_offsets.at(id);

        surfacePoints.push_back(extractHomogeneous(transform * Scalar(0, 0, 0, 1)));
        surfacePoints.push_back(extractHomogeneous(transform * Scalar(size, 0, 0, 1)));
        surfacePoints.push_back(extractHomogeneous(transform * Scalar(size, size, 0, 1)));
        surfacePoints.push_back(extractHomogeneous(transform * Scalar(0, size, 0, 1)));

        imagePoints.push_back(detections.at(i).getCorner(0));
        imagePoints.push_back(detections.at(i).getCorner(1));
        imagePoints.push_back(detections.at(i).getCorner(2));
        imagePoints.push_back(detections.at(i).getCorner(3));
    }

    solvePnP(surfacePoints, imagePoints, intrinsics, distortion, rotVec, translation);
    rotVec.convertTo(rotVec, CV_32F);
    translation.convertTo(translation, CV_32F);
    Rodrigues(rotVec, rotation);

    Mat extrinsics;
    hconcat(rotation, translation, extrinsics);
    Mat projection = intrinsics * extrinsics;
    projection.col(0).copyTo(homography.col(0));
    projection.col(1).copyTo(homography.col(1));
    projection.col(3).copyTo(homography.col(2));

    return true;
}

}
