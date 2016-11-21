

#include "camera.h"
#include "definitions.h"
#include "json.h"

#include "debug.h"
#include "utilities.h"

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
        sleep(50);
    }

	return NULL;
}

CameraHandler::CameraHandler(int id, const string& tmpl, bool debug) : device(NULL), frame_counter(0), debug(debug), reposition_throttle(10) {

    char* buffer;
    size_t size;

	string template_filename = string("template_") + tmpl + string(".json");

    if (!get_resource(template_filename, &buffer, &size))
        throw runtime_error("Unable to load calibration template data: not found");

	Json::Reader reader;
	Json::Value template_root;

	if (!reader.parse(string(buffer, size), template_root) || !template_root.isArray()) {
		free(buffer);
		throw runtime_error("Unable to load calibration template data: parse error");
	}

	for (int i = 0; i < template_root.size(); i++) {
		Json::Value template_description = template_root[i];

		if (!template_description.isObject()) continue;
		int template_size = template_description["size"].asInt();
		float template_orientation = M_PI * (template_description["orientation"].asFloat()) / 180;
		float template_rotation = M_PI * (template_description["rotation"].asFloat()) / 180;

		Matx44f rotate = translateMatrix(-template_size/2, -template_size/2, 0) * rotateMatrix(0, 0, template_orientation) * translateMatrix(template_size/2, template_size/2, 0);
		detector.loadPattern(template_description["file"].asString(), template_size);
		float ox = template_description["origin"]["x"].asFloat();
		float oy = template_description["origin"]["y"].asFloat();
		float oz = template_description["origin"]["z"].asFloat();
//cout << template_description["file"].asString();
//cout << ox << " " << oy << " " << oz << " " << template_orientation << " " << template_rotation << endl;

		pattern_offsets.push_back(rotate * translateMatrix(ox, oy, oz) * rotateMatrix(0, 0, template_rotation));
	}

	free(buffer);

    if (!get_resource("calibration.xml", &buffer, &size))
        throw runtime_error("Unable to load intrinsic calibration data");

    FileStorage fs(string(buffer, size), FileStorage::READ + FileStorage::MEMORY);
    fs["intrinsic"] >> intrinsics;
    fs["distortion"] >> distortion;
    free(buffer);

    device = new VideoCapture(id);

    if (!device->isOpened())
        throw runtime_error("Camera not available");

	device->set(CV_CAP_PROP_SETTINGS, 1); //set(CV_PROP_AUTOFOCUS, 0);


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

    } else if (command == "describe") {

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
        trans["m1"] = Json::Value(translation.at<float>(1, 0));
        trans["m2"] = Json::Value(translation.at<float>(2, 0));

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

	bool reposition = frame_counter % reposition_throttle == 0;

    if (temp.empty()) {
        if (frame_counter % 100 == 0) {
            DEBUGMSG("Unable to retrieve image.");
        }
        return false;
    }

	if (debug && reposition)
		temp.copyTo(debug_frame);

    MUTEX_UNLOCK(camera_mutex);

    if (reposition) {
        vector<PatternDetection> detectedPatterns;
        detector.detect(temp, intrinsics, distortion, detectedPatterns);

		if (debug) {
			for (size_t i = 0; i<detectedPatterns.size(); i++) {
				Matx44f origin = pattern_offsets.at(detectedPatterns.at(i).getIdentifier());
				detectedPatterns.at(i).draw(debug_frame, intrinsics, distortion, origin.inv());
			}
		}

        MUTEX_LOCK(camera_mutex);

        if (detectedPatterns.size() > 0) {

            localize_camera(detectedPatterns);

        }

        MUTEX_UNLOCK(camera_mutex);

    }

    MUTEX_LOCK(camera_mutex);
    temp.copyTo(frame);
    MUTEX_UNLOCK(camera_mutex);

	if (debug && reposition)  {
		imshow("Camera debug", debug_frame);
		waitKey(1);
	}

    return true;
}

bool CameraHandler::localize_camera(vector<PatternDetection> detections)
{

    if (detections.size() < 1) return false;

    PatternDetection* anchor = NULL;

    Mat rotVec;

    vector<Point3f> surfacePoints;
    vector<Point2f> imagePoints;

	Matx44f global = rotateMatrix(0, 0, M_PI);

    for (unsigned int i =0; i < detections.size(); i++) {

        int id = detections.at(i).getIdentifier();

        if (id >= pattern_offsets.size())
            continue;

        float size = (float) detections.at(i).getSize();

        Matx44f transform = pattern_offsets.at(id);

        surfacePoints.push_back(extractHomogeneous(global * transform * Scalar(0, 0, 0, 1)));
        surfacePoints.push_back(extractHomogeneous(global * transform * Scalar(size, 0, 0, 1)));
        surfacePoints.push_back(extractHomogeneous(global * transform * Scalar(size, size, 0, 1)));
        surfacePoints.push_back(extractHomogeneous(global * transform * Scalar(0, size, 0, 1)));

        imagePoints.push_back(detections.at(i).getCorner(0));
        imagePoints.push_back(detections.at(i).getCorner(1));
        imagePoints.push_back(detections.at(i).getCorner(2));
        imagePoints.push_back(detections.at(i).getCorner(3));
    }

	//if (surfacePoints.size() > 4) {
	//	DEBUGMSG("Estimating plane on %d points\n", (int) surfacePoints.size());
	//	solvePnPRansac(surfacePoints, imagePoints, intrinsics, distortion, rotVec, translation, false, 100, 13.0, std::max(8, (int) surfacePoints.size() / 2));
	//} else {
	    solvePnP(surfacePoints, imagePoints, intrinsics, distortion, rotVec, translation);
	//}
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
