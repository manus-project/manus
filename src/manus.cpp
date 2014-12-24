
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

#include "server.h"
#include "filesystem.h"
#include "json.h"
#include "debug.h"
#include "threads.h"
#include "simulation.h"
#include "serial.h"

#define APPLICATION_NAME "Manus"
#define APPLICATION_VERSION "0.1"

#ifdef _WIN32
    #include <windows.h>
    #include <direct.h>  // For chdir()
    #include <winsvc.h>
    #include <shlobj.h>

    #ifndef PATH_MAX
    #define PATH_MAX MAX_PATH
    #endif

    #ifndef S_ISDIR
    #define S_ISDIR(x) ((x) & _S_IFDIR)
    #endif

    #define DIRSEP '\\'
    #define snprintf _snprintf
    #define vsnprintf _vsnprintf
    #define sleep(x) Sleep((x))
    #define abs_path(rel, abs, abs_size) _fullpath((abs), (rel), (abs_size))
    #define SIGCHLD 0
    typedef struct _stat file_stat_t;
    #define stat(x, y) _stat((x), (y))
#else
    typedef struct stat file_stat_t;
    #include <sys/wait.h>
    #include <unistd.h>

    #define DIRSEP '/'
    #define __cdecl
    #define abs_path(rel, abs, abs_size) realpath((rel), (abs))
	#define sleep(x) usleep((x) * 1000)
#endif // _WIN32

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
#include "embedded.h"
#include <algorithm>

class FilesHandler : public Handler {
public:
    FilesHandler() {};
    ~FilesHandler() {};

    virtual void handle(Request& request) {
        
        const char* data = (request.get_uri() == "/") ? embedded_get("/index.html") : embedded_get(request.get_uri().c_str());

        if (data) {
            request.set_status(200);
            printf("Serving %s\n", request.get_uri().c_str());
            request.send_data(data, strlen(data));
        } else {
            request.set_status(404);      
        }

        request.finish();

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
            printf("Serving %s\n", path.c_str());
            request.send_data(buffer, length);
            free(buffer);
        } else {
            request.set_status(404); 
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

#define CONVERT_OUTGOING_VALUE(T, V) ((T == ROTATION) ? ((V / 180) * M_PI) : V)
#define CONVERT_INCOMING_VALUE(T, V) ((T == ROTATION) ? ((V * 180) / M_PI) : V)

#define DEGREE_TO_RADIAN(X) ((X / 180) * M_PI)
#define RADIAN_TO_DEGREE(X) ((X * 180) / M_PI)

class ArmApiHandler : public Handler {
public:
    ArmApiHandler(RobotArm* arm) : arm(arm) {
        MUTEX_INIT(mutex);
    };

    ~ArmApiHandler() {
        MUTEX_DESTROY(mutex);
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

        if (command == "describe") {

            ArmInfo info;

            MUTEX_LOCK(mutex);
            arm->getArmInfo(info);

            response["version"] = Json::Value(info.version);
            response["name"] = Json::Value(info.name);

            Json::Value joints;

            for (int i = 0; i < info.joints; i++) {
                JointInfo jointInfo;
                if (arm->getJointInfo(i, jointInfo) < 0) break;

                Json::Value joint;
                joint["type"] = Json::Value(jointTypeToString(jointInfo.type));
                joint["alpha"] = Json::Value(DEGREE_TO_RADIAN(jointInfo.dh_alpha));
                joint["a"] = Json::Value(jointInfo.dh_a);
                joint["theta"] = Json::Value(DEGREE_TO_RADIAN(jointInfo.dh_theta));
                joint["d"] = Json::Value(jointInfo.dh_d);
                joint["min"] = Json::Value(CONVERT_OUTGOING_VALUE(jointInfo.type, jointInfo.dh_min));
                joint["max"] = Json::Value(CONVERT_OUTGOING_VALUE(jointInfo.type, jointInfo.dh_max));


                joints[i] = joint;
            }

            MUTEX_UNLOCK(mutex);

            response["joints"] = joints;

        } else if (command == "status") {

            ArmData data;

            MUTEX_LOCK(mutex);
            arm->getArmData(data);

            response["state"] = Json::Value(armStateToString(data.state));
            Json::Value positions;
            Json::Value goals;

            for (int i = 0; i < arm->size(); i++) {
                JointData jointData;
                arm->getJointData(i, jointData);
                JointInfo jointInfo;
                if (arm->getJointInfo(i, jointInfo) < 0) break;
                positions[i] = Json::Value(CONVERT_OUTGOING_VALUE(jointInfo.type, jointData.dh_position));
                goals[i] = Json::Value(CONVERT_OUTGOING_VALUE(jointInfo.type, jointData.dh_goal));
            }
            MUTEX_UNLOCK(mutex);

            response["joints"] = positions;
            response["goals"] = goals;

        } else if (command == "move") {
            
            try {

                int joint = stoi(request.get_variable("joint"));

                float value = stof(request.get_variable("position"));

                float speed = stof(request.get_variable("speed"));
                if (speed == 0) speed = 0.1;

                JointInfo jointInfo;
                arm->getJointInfo(joint, jointInfo);

                MUTEX_LOCK(mutex);
                int result = arm->moveTo(joint, speed, CONVERT_INCOMING_VALUE(jointInfo.type, value));
                MUTEX_UNLOCK(mutex);

                if (result >= 0)
                    response["success"] = Json::Value(true);
                else {
                    response["success"] = Json::Value(false);
                }

            } catch (invalid_argument) {
                response["success"] = Json::Value(false);
            }

        } else {
            response["success"] = Json::Value(false);
        }

        Json::FastWriter writer;
        request.send_data(writer.write(response));

        request.finish();
    }

    void poll() {

        MUTEX_LOCK(mutex);
        arm->poll();
        MUTEX_UNLOCK(mutex);

    }

private:

    THREAD_MUTEX mutex;

    RobotArm* arm;

};

THREAD_CALLBACK(arm_driver_function, handler) {

    ArmApiHandler* arm_handler = (ArmApiHandler*) handler;

    while (exit_flag == 0) {
        arm_handler->poll();
        sleep(100);
    }

	return NULL;
}


static void show_usage_and_exit(void) {
  const char **names;
  int i;

  fprintf(stderr, "%s version %s (c) ViCoS Lab, built on %s\n",
          APPLICATION_NAME, APPLICATION_VERSION, __DATE__);
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "  manus [arm_serial_port]\n");

  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {

    THREAD arm_thread;
__debug_enable();
    RobotArm* arm = NULL;

    if (argc > 1) {
        DEBUGMSG("Starting USB driver\n");
        arm = new SerialPortRobotArm(string(argv[1]));
    } else {
        DEBUGMSG("Starting simulation driver\n");
        arm = new SimulatedRobotArm();
    }

    if (arm->connect() < 0) {
        printf("Unable to connect to robot arm \n");
        return -1;
    }

    if (arm->startControl() < 0) {
        printf("Unable to connect to robot arm \n");
        return -2;
    }

    FilesHandler files_handler;
    ArmApiHandler arm_handler(arm);
    AppApiHandler app_handler;

    Server server;
    server.set_default_handler(&files_handler);
    server.append_handler("/api/arm", &arm_handler);
    server.append_handler("/api/app", &app_handler);

    // Setup signal handler: quit on Ctrl-C
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
#ifndef _WIN32
    signal(SIGCHLD, signal_handler);
#endif

    CREATE_THREAD(arm_thread, arm_driver_function, &arm_handler);

    printf("%s serving on port http://localhost:%d/\n", APPLICATION_NAME, server.get_port());
    fflush(stdout);  // Needed, Windows terminals might not be line-buffered

    while (exit_flag == 0) {
        server.wait(1000);
    }

    printf("Exiting on signal %d ...", exit_flag);
    fflush(stdout);
    printf("%s\n", " done.");

    if (arm) {
        arm->stopControl();
        arm->disconnect();
        delete arm;
    }

    RELEASE_THREAD(arm_thread);

    return EXIT_SUCCESS;
}

