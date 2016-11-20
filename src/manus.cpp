
#define _USE_MATH_DEFINES

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include <math.h>

#include <regex>
#include <iostream>
#include <stdexcept>
#include <ctime>
#include <map>

#include "server.h"
#include "filesystem.h"
#include "json.h"
#include "debug.h"
#include "threads.h"
#include "simulation.h"
#include "markers.h"
#include "serial.h"
#include "camera.h"
#include "getopt.h"

using namespace manus;

#define APPLICATION_NAME "Manus"
#define APPLICATION_OPTIONS "hdc:a:"

#ifndef APPLICATION_VERSION
#define APPLICATION_VERSION "dev"
#endif


// http://www.codeproject.com/Articles/74/Adding-Icons-to-the-System-Tray

#include "definitions.h"

static int exit_flag;

static void __cdecl signal_handler(int sig_num) {
  // Reinstantiate signal handler
  signal(sig_num, signal_handler);

#ifndef _WIN32
  // Do not do the trick with ignoring SIGCHLD, cause not all OSes (e.g. QNX)
  // reap zombies if SIGCHLD is ignored. On QNX, for example, waitpid()
  // fails if SIGCHLD is ignored, making system() non-functional.
  if (sig_num == SIGCHLD) {
    do {} while (waitpid(-1, &sig_num, WNOHANG) > 0);
  } else
#endif
  { exit_flag = sig_num; }
}

#ifndef SERVER_DOCUMENT_ROOT
#define embedded_header_only
#include "embedded.c"
#include <algorithm>

class FilesHandler : public Handler {
public:
    FilesHandler() {};
    ~FilesHandler() {};

    virtual void handle(Request& request) {

        string name;

		if (request.get_uri() == "/")
			name = "index.html";
		else
			name = request.get_uri().substr(1);

		if (embedded_has_resource(name.c_str())) {
            request.set_status(200);
            request.set_header("Cache-Control", "max-age=0, post-check=0, pre-check=0, no-store, no-cache, must-revalidate");
            DEBUGMSG("Serving %s\n", name.c_str());
            embedded_get_resource(name.c_str(), &FilesHandler::serve_data, &request);
        } else {
            DEBUGMSG("Not found %s\n", request.get_uri().c_str());
            request.set_status(404);
            request.send_data("Not found");
        }

        request.finish();

    }

private:

    static int serve_data(const void* buffer, int len, void *user) {
        Request* request = (Request*) user;
        request->send_data(buffer, len);
        return 1;
    }

};

#else

class FilesHandler : public Handler {
public:
    FilesHandler() {};
    ~FilesHandler() {};

    virtual void handle(Request& request) {

        string path = request.get_uri().substr(1);

        if (path.empty())
            path = "index.html";

        std::replace( path.begin(), path.end(), '/', FILE_DELIMITER);

        path = path_join(SERVER_DOCUMENT_ROOT, path);

        if (file_type(path) == FILETYPE_FILE) {
            char* buffer;
            int length = file_read(path, &buffer);
            DEBUGMSG("Serving external %s\n", path.c_str());
            request.send_data(buffer, length);
            free(buffer);
        } else {
            DEBUGMSG("Not found %s\n", request.get_uri().c_str());
            request.set_status(404);
            request.send_data("Not found");
        }

        request.finish();

    }

};

#endif

class AppApiHandler : public Handler {
public:
    AppApiHandler() {

    };

    ~AppApiHandler() {

    };

    virtual void handle(Request& request) {

        if (!request.has_variable("command")) {
            request.set_status(404);
            request.finish();
            return;
        }

        string command = request.get_variable("command");

        request.set_status(200);
        request.set_header("Content-Type", "application/json");
        request.set_header("Cache-Control", "max-age=0, post-check=0, pre-check=0, no-store, no-cache, must-revalidate");

        Json::Value response;

        if (command == "information") {

            response["name"] = Json::Value(APPLICATION_NAME);
            response["version"] = Json::Value(APPLICATION_VERSION);
            response["build"] = Json::Value(__DATE__);

        } else {
            response["success"] = Json::Value(false);
        }

        Json::FastWriter writer;
        request.send_data(writer.write(response));

        request.finish();
    }



};

static void show_usage_and_exit(void) {

  fprintf(stderr, "%s version %s (c) ViCoS Lab, built on %s\n",
          APPLICATION_NAME, APPLICATION_VERSION, __DATE__);
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "  manus [-h] [-d] [-c camera_id] [-a serial_port]\n");

  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {

    int c;
    int camera_id = -1;
    string arm_address;
	std::map<string, shared_ptr<Handler> > handlers;

    while ((c = getopt(argc, argv, APPLICATION_OPTIONS)) != -1)
        switch (c) {
        case 'h':
            show_usage_and_exit();
            exit(0);
        case 'd':
            __debug_enable();
            break;
        case 'c':
            camera_id = atoi(optarg);
            break;
        case 'a':
            arm_address = std::string(optarg);
            break;
        default:
            show_usage_and_exit();
            throw std::runtime_error(string("Unknown switch -") + string(1, (char) optopt));
        }

    RobotArm* arm = NULL;

    if (!arm_address.empty()) {
		if (arm_address != "sim") {
	        DEBUGMSG("Starting USB driver\n");
    	    arm = new SerialPortRobotArm(arm_address);
	    } else {
    	    DEBUGMSG("Starting simulation driver\n");
    	    arm = new SimulatedRobotArm();
	    }
	}

	if (arm) {
		if (arm->connect() < 0) {
		    printf("Unable to connect to robot arm \n");
		    return -1;
		}

		if (arm->startControl() < 0) {
		    printf("Unable to start control of robot arm \n");
		    return -2;
		}
	}

    handlers["files"] = make_shared<FilesHandler>();
    handlers["app"] = make_shared<AppApiHandler>();
	handlers["markers"] = make_shared<MarkersApiHandler>();

    shared_ptr<Server> server = make_shared<Server>();
    server->set_default_handler(handlers["files"]);
    server->append_handler(make_shared<PrefixMatcher>("/api/app/", "command"), handlers["app"]);
	server->append_handler(make_shared<PrefixMatcher>("/api/markers/", "command"), handlers["markers"]);

	if (arm) {
	    handlers["arm"] = make_shared<ArmApiHandler>(arm);
    	server->append_handler(make_shared<PrefixMatcher>("/api/arm/", "command"), handlers["arm"]);
	}

    if (camera_id > -1) {
        handlers["camera"] = make_shared<CameraHandler>(camera_id);
        server->append_handler(make_shared<PrefixMatcher>("/api/camera/", "command"), handlers["camera"]);
    }

    // Setup signal handler: quit on Ctrl-C
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
#ifndef _WIN32
    signal(SIGCHLD, signal_handler);
#endif

    printf("%s serving on port http://localhost:%d/\n", APPLICATION_NAME, server->get_port());
    printf("Press Ctrl-C to exit.\n");
    fflush(stdout);  // Needed, Windows terminals might not be line-buffered

    while (exit_flag == 0) {
        server->wait(1000);
    }

    DEBUGMSG("Received signal %d.\n", exit_flag);
    printf("Exiting.\n");

    server.reset();

	for (std::map<std::string, shared_ptr<Handler> >::iterator it = handlers.begin();
			it != handlers.end(); it ++) {
		it->second.reset();
	}

    if (arm) {
        arm->stopControl();
        arm->disconnect();
        delete arm;
    }

    return EXIT_SUCCESS;
}

