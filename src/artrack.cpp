#include <echolib/opencv.h>

#include <chrono>
#include <memory>
#include <experimental/filesystem>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/video/video.hpp>
#ifdef MANUS_DEBUG
#include <opencv2/highgui/highgui.hpp>
#endif

#include <ary/complex.h>
#include <ary/utilities.h>

using namespace std;
using namespace cv;
using namespace echolib;
using namespace ary;

bool debug = false;


Ptr<CameraModel> model;
Ptr<Scene> scene;
Header header_current;
Mat image_current;
Mat image_gray;
Ptr<BackgroundSubtractorMOG2> scene_model;
int force_update_counter;
int force_update_threshold;
SharedLocalization localization;
SharedCameraModel parameters(new CameraModel(Size(640, 480)));

bool scene_change(Mat& image) {

	setDrawScale(1000);

	if (image.empty())
		return false;

	Mat candidate, mask;
	resize(image, candidate, Size(64, 64));

	if (scene_model.empty()) {
#if CV_MAJOR_VERSION == 3
		scene_model = cv::createBackgroundSubtractorMOG2();
		scene_model->setHistory(100);
		scene_model->setNMixtures(4);
		scene_model->setDetectShadows(false);
#else
		scene_model = Ptr<BackgroundSubtractorMOG2>(new BackgroundSubtractorMOG2(100, 4, false));
#endif
	}

	if (force_update_counter > force_update_threshold) {
		force_update_counter = 0;
		return true;
	}

	force_update_counter++;
#if CV_MAJOR_VERSION == 3
	scene_model->apply(candidate, mask);
#else
	scene_model->operator()(candidate, mask);
#endif

	#ifdef MANUS_DEBUG
	if (debug) {
		imshow("Changes", mask);
	}
	#endif

	float changes = (float)(sum(mask)[0]) / (float)(mask.cols * mask.rows * 255);

	if (changes > 0.2) {
		force_update_counter = 0;
		return true;
	} else {
		return false;
	}
}

void handle_frame(shared_ptr<Frame> frame) {

	if (frame->image.empty())
		return;

	header_current = frame->header;
	image_current = frame->image;

}

SharedTypedPublisher<CameraExtrinsics> location_publisher;
SharedTypedSubscriber<CameraIntrinsics> parameters_listener;

vector<Point3f> blobs;

void read_blobs(const char* file) {

	blobs.clear();

	FileStorage fs(file, FileStorage::READ);

	if (!fs.isOpened()) {
		return;
	}

    FileNode centers = fs["centers"];

	for (FileNodeIterator it = centers.begin(); it != centers.end(); ++it) {

		float x, y;

		read((*it)["x"], x, 0);
		read((*it)["y"], y, 0);

		blobs.push_back(Point3f(x, y, 0));

	}

}

#ifdef MANUS_DEBUG
SharedLocalization improveLocalizationWithKeypointBlobs(SharedLocalization localization, SharedCameraModel camera, Mat debug = Mat())
#else
SharedLocalization improveLocalizationWithKeypointBlobs(SharedLocalization localization, SharedCameraModel camera) 
#endif
{

	vector<Point2f> estimates;

	projectPoints(blobs, localization->getCameraPosition().rotation, localization->getCameraPosition().translation, 
		camera->getIntrinsics(), camera->getDistortion(), estimates);

#if CV_MAJOR_VERSION == 3
	Ptr<FeatureDetector> blobDetector = SimpleBlobDetector::create();
#else
	Ptr<FeatureDetector> blobDetector = FeatureDetector::create("SimpleBlob");
#endif

	std::vector<KeyPoint> keypoints;
    blobDetector->detect(image_gray, keypoints);
    std::vector<Point2f> points;
    for (size_t i = 0; i < keypoints.size(); i++)
    {
      	points.push_back (keypoints[i].pt);
	}

	vector<Point2f> imagePoints;
	vector<Point3f> objectPoints;

	for (int i = 0; i < estimates.size(); i++) {

		float min_distance = 100000;
		int best_match = -1;

		// Let's skip all points that are projected out of frame
		if (estimates[i].x < 0 || estimates[i].y < 0 ||
			estimates[i].x >= image_gray.cols || estimates[i].y >= image_gray.rows)
			continue;

		for (int j = 0; j < points.size(); j++) {
			
			float dx = estimates[i].x - points[j].x;
			float dy = estimates[i].y - points[j].y;
			float dist = sqrt (dx * dx + dy * dy);

			if (dist < 20) {

				imagePoints.push_back(points[j]);
				objectPoints.push_back(blobs[i]);

				#ifdef MANUS_DEBUG
				if (!debug.empty()) {
					circle(debug, estimates[i], 3, Scalar(255, 255, 0));
					circle(debug, points[j], 3, Scalar(255, 0, 0));
					line(debug, estimates[i], points[j], Scalar(0, 255, 255), 1);
				}
				#endif

			}

		}

	}

	if (imagePoints.size() > 5) {

		CameraPosition position;

		Mat rotation, translation;

		vector<int> inliers;

#if CV_MAJOR_VERSION == 3
		solvePnPRansac(objectPoints, imagePoints, camera->getIntrinsics(), camera->getDistortion(), rotation, translation, false, 200, 4, 0.999, inliers);
#else
		solvePnPRansac(objectPoints, imagePoints, camera->getIntrinsics(), camera->getDistortion(), rotation, translation, false, 200, 4, imagePoints.size(), inliers);
#endif
		rotation.convertTo(position.rotation, CV_32F);
		translation.convertTo(position.translation, CV_32F);

		float err = cv::norm(localization->getCameraPosition().translation, position.translation);

		// Some heuristic criteria
		if (err < 100 && inliers.size() > imagePoints.size() / 2)
			localization = Ptr<Localization>(new Localization(0, position));

	}

	return localization;

}

int main(int argc, char** argv) {

	if (argc < 2) {
		cerr << "No AR board description specified." << endl;
		exit(-1);
	}

	string blobs_file;

	string scene_file(argv[1]);

	cout << "AR scene file: " << scene_file << endl;

	if (argc > 2) {
		blobs_file = string(argv[2]);
	}


	if (!blobs_file.empty()) {
		read_blobs(blobs_file.c_str());
		cout << "Blobs file: " << blobs_file << endl;
	}

#ifdef MANUS_DEBUG
	Mat debug_image;
	debug = getenv("SHOW_DEBUG") != NULL;
#endif

	bool localized = false;

	CameraExtrinsics last_location;

	force_update_threshold = 100;
	force_update_counter = force_update_threshold;

	SharedClient client = echolib::connect(string(), "artracker");

	SharedTypedSubscriber<Frame> sub;
	location_publisher = make_shared<TypedPublisher<CameraExtrinsics> >(client, "location");

	parameters_listener = make_shared<TypedSubscriber<CameraIntrinsics> >(client, "intrinsics",
	[&scene_file](shared_ptr<CameraIntrinsics> p) {
		parameters = Ptr<CameraModel>(new CameraModel(p->intrinsics, p->distortion));
		parameters_listener.reset();
		if (!scene)
			scene = Ptr<Scene>(new Scene(parameters, scene_file));
	});

	bool processing = false;

    SubscriptionWatcher watcher(client, "location", [&processing, &sub, &client](int subscribers) {
		processing = subscribers > 0;

		if (processing && !sub) {
			sub = make_shared<TypedSubscriber<Frame> >(client, "camera", handle_frame);
		}
		if (!processing && sub && !debug) {
			sub.reset();
		}

	});

	if (debug) {
		sub = make_shared<TypedSubscriber<Frame> >(client, "camera", handle_frame);
	}

	while (true) {

		if (!echolib::wait(100))
			break;

		if (!image_current.empty()) {

			if (scene_change(image_current)) {

				cvtColor(image_current, image_gray, CV_BGR2GRAY);

				vector<SharedLocalization> anchors;

				if (scene)
					anchors = scene->localize(image_gray);

				if (anchors.size() > 0) {
					localization = anchors[0];
					localized = true;
				} else {
					localized = false;
				}

				#ifdef MANUS_DEBUG
				if (debug) {
					image_current.copyTo(debug_image);
				}
				#endif

				if (blobs.size() > 0 && localized) {

					#ifdef MANUS_DEBUG
					localization = improveLocalizationWithKeypointBlobs(localization, parameters, debug_image);
					#else
					localization = improveLocalizationWithKeypointBlobs(localization, parameters);
					#endif
				}

				#ifdef MANUS_DEBUG
				if (debug && localized) {
					localization->draw(debug_image, parameters);
				}
				#endif

				#ifdef MANUS_DEBUG
				if (debug) {
					imshow("AR Track", debug_image);
				}
				#endif

			}
			
			if (localized) {		
				last_location.header = header_current;
				Rodrigues(localization->getCameraPosition().rotation, last_location.rotation);
				last_location.translation = localization->getCameraPosition().translation;
				location_publisher->send(last_location);
			}

			image_current.release();

		}

#ifdef MANUS_DEBUG
		if (debug) {
			int k = waitKey(1);
			if ((char)k == 'r' && scene) {
				cout << "Reloading markers" << endl;
				scene = Ptr<Scene>(new Scene(parameters, scene_file));
			}
		}
#endif
	}

	exit(0);
}
