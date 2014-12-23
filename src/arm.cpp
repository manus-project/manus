
#include "arm.h"

string armStateToString(ArmState status) {

    switch (status) {
        case UNKNOWN: return "unknown";
        case CONNECTED: return "connected";
        case PASSIVE: return "passive";
        case ACTIVE: return "active";
        case CALIBRATION: return "calibration";
    }

    return "unkwnown";
}

string jointTypeToString(JointType type) {

    switch (type) {
        case ROTATION: return "rotation";
        case TRANSLATION: return "translation";
        case GRIPPER: return "gripper";
    }

    return "unkwnown";
}

JointInfo createJointInfo(int id, JointType type, float dh_theta, float dh_alpha, float dh_d, float dh_a, float min, float max) {
    JointInfo joint;
	joint.joint_id = id;
    joint.type = type;
    joint.dh_theta = dh_theta;
    joint.dh_alpha = dh_alpha;
    joint.dh_d = dh_d;
    joint.dh_r = dh_a;
	joint.position_min = min;
	joint.position_max = max;
    return joint;
}

JointData createJointData(int id, float position) {
    JointData joint;
	joint.joint_id = id;
    joint.position = position;
    joint.position_goal = position;
    return joint;
}
