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
Ptr<BackgroundSubtractorMOG2> scene_model;
int force_update_counter;
int force_update_threshold;
SharedLocalization location;
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

int main(int argc, char** argv) {

	if (argc < 2) {
		cerr << "No AR board description specified." << endl;
		exit(-1);
	}

	string scene_file(argv[1]);

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

				vector<SharedLocalization> anchors;

				if (scene)
					anchors = scene->localize(image_current);

				if (anchors.size() > 0) {
					location = anchors[0];
					localized = true;
				} else {
					localized = false;
				}

				#ifdef MANUS_DEBUG
				if (debug) {
					image_current.copyTo(debug_image);

					location->draw(debug_image, parameters);

					imshow("AR Track", debug_image);
				}
				#endif

			}
			if (localized) {
				CameraExtrinsics loc;
				Rodrigues(location->getCameraPosition().rotation, loc.rotation);
				loc.translation = location->getCameraPosition().translation;
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
