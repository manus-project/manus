
#include <iostream>
#include <cmath>

#include "debug.h"
#include "arm.h"
#include "json.h"
#include "definitions.h"

using namespace std;

string armStateToString(ArmState status) {

    switch (status) {
        case UNKNOWN: return "unknown";
        case CONNECTED: return "connected";
        case PASSIVE: return "passive";
        case ACTIVE: return "active";
        case CALIBRATION: return "calibration";
    }

    return "unknown";
}

string jointTypeToString(JointType type) {

    switch (type) {
        case ROTATION: return "rotation";
        case TRANSLATION: return "translation";
        case FIXED: return "fixed";
        case GRIPPER: return "gripper";
    }

    return "unknown";
}

JointInfo createJointInfo(int id, JointType type, float dh_theta, float dh_alpha, float dh_d, float dh_a, float min, float max) {
    JointInfo joint;
	joint.joint_id = id;
    joint.type = type;
    joint.dh_theta = dh_theta;
    joint.dh_alpha = dh_alpha;
    joint.dh_d = dh_d;
    joint.dh_a = dh_a;
	joint.dh_min = min;
	joint.dh_max = max;
	joint.position_min = min;
	joint.position_max = max;
    return joint;
}

JointData createJointData(int id, float position) {
    JointData joint;
	joint.joint_id = id;
    joint.position = position;
    joint.position_goal = position;
    joint.dh_position = position;
    joint.dh_goal = position;
    return joint;
}

double normalizeAngle(double val, double min, double max) {
	if (val > max) {
		//Find actual angle offset
		double diffangle = std::fmod(val-max,2*M_PI);
		// Add that to upper bound and go back a full rotation
		val = max + diffangle - 2*M_PI;
	}

	if (val < min) {
		//Find actual angle offset
		double diffangle = std::fmod(min-val,2*M_PI);
		// Add that to upper bound and go back a full rotation
		val = min - diffangle + 2*M_PI;
	}

	return val;
}

#define CONVERT_OUTGOING_VALUE(T, V) ((T == ::ROTATION) ? ((V / 180) * M_PI) : V)
#define CONVERT_INCOMING_VALUE(T, V) ((T == ::ROTATION) ? ((V * 180) / M_PI) : V)

#define DEGREE_TO_RADIAN(X) ((X / 180) * M_PI)
#define RADIAN_TO_DEGREE(X) ((X * 180) / M_PI)


THREAD_CALLBACK(arm_driver_function, handler) {

    ArmApiHandler* arm_handler = (ArmApiHandler*) handler;

    while (true) {
        if (!arm_handler->poll()) break;
        sleep(100);
    }

	return NULL;
}


ArmApiHandler::ArmApiHandler(RobotArm* arm) : arm(arm) {
    MUTEX_INIT(mutex);

    CREATE_THREAD(thread, arm_driver_function, this);
};

ArmApiHandler::~ArmApiHandler() {
    MUTEX_DESTROY(mutex);
    RELEASE_THREAD(thread);
};

void ArmApiHandler::handle(Request& request) {

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

			int result = -1;

            MUTEX_LOCK(mutex);
			if (jointInfo.type == ROTATION) {
				value = normalizeAngle(value, jointInfo.dh_min, jointInfo.dh_max);
				value = CONVERT_INCOMING_VALUE(jointInfo.type, value);
			}

			if (value <= jointInfo.dh_max && value >= jointInfo.dh_min) {
            	result = arm->moveTo(joint, speed, value);
			} else {
				DEBUGMSG("Value %f for joint %d out of range [%f, %f]\n", value, joint, jointInfo.dh_min, jointInfo.dh_max);
			}
            MUTEX_UNLOCK(mutex);

            if (result >= 0)
                response["success"] = Json::Value(true);
            else {
                response["success"] = Json::Value(false);
            }

        } catch (invalid_argument) {
            response["success"] = Json::Value(false);
        }

    } else if (command == "start") {
        MUTEX_LOCK(mutex);
        int result = arm->startControl();
        MUTEX_UNLOCK(mutex);

        response["success"] = Json::Value(result == 0);

    } else if (command == "stop") {

        MUTEX_LOCK(mutex);
        int result = arm->stopControl();
        MUTEX_UNLOCK(mutex);

        response["success"] = Json::Value(result == 0);

    } else {
        response["success"] = Json::Value(false);
    }

    Json::FastWriter writer;
    request.send_data(writer.write(response));

    request.finish();
}

bool ArmApiHandler::poll() {

    bool result = false;

    MUTEX_LOCK(mutex);

        if (arm->isConnected())
            result = arm->poll();
        else result = false;

    MUTEX_UNLOCK(mutex);

    return result;

}

