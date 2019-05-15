#include <echolib/opencv.h>

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
		scene_model = Ptr<BackgroundSubtractorMOG2>(new BackgroundSubtractorMOG2(100, 4, false));
	}

	if (force_update_counter > force_update_threshold) {
		force_update_counter = 0;
		return true;
	}

	force_update_counter++;

	scene_model->operator()(candidate, mask);

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

void handle_frame(Mat& image) {

	if (image.empty())
		return;

	image_current = image;

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

SharedLocalization improveLocalizationWithKeypointBlobs(SharedLocalization localization, SharedCameraModel camera) {
	vector<Point2f> estimates;

	projectPoints(blobs, localization->getCameraPosition().rotation, localization->getCameraPosition().translation, 
		camera->getIntrinsics(), camera->getDistortion(), estimates);

	Ptr<FeatureDetector> blobDetector = FeatureDetector::create("SimpleBlob");

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
			}

		}

	}

	if (imagePoints.size() > 5) {

		CameraPosition position;

		Mat rotation, translation;

		solvePnPRansac(objectPoints, imagePoints, camera->getIntrinsics(), camera->getDistortion(), rotation, translation);

		rotation.convertTo(position.rotation, CV_32F);
		translation.convertTo(position.translation, CV_32F);

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

	if (argc > 2) {
		blobs_file = string(argv[2]);
	}

	string scene_file(argv[1]);

	if (!blobs_file.empty()) {
		read_blobs(blobs_file.c_str());
	}

#ifdef MANUS_DEBUG
	Mat debug_image;
	debug = getenv("SHOW_DEBUG") != NULL;
#endif

	bool localized = false;

	force_update_threshold = 100;
	force_update_counter = force_update_threshold;

	SharedClient client = echolib::connect();

	shared_ptr<ImageSubscriber> sub;
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
			sub = make_shared<ImageSubscriber>(client, "camera", handle_frame);
		}
		if (!processing && sub && !debug) {
			sub.reset();
		}
	});

	if (debug) {
		sub = make_shared<ImageSubscriber>(client, "camera", handle_frame);
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

					localization = improveLocalizationWithKeypointBlobs(localization, parameters);

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
				CameraExtrinsics loc;
				Rodrigues(localization->getCameraPosition().rotation, loc.rotation);
				loc.translation = localization->getCameraPosition().translation;
				location_publisher->send(loc);
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
