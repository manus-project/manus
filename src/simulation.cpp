
#include "simulation.h"

SimulatedRobotArm::SimulatedRobotArm() {

    arm_data.state = UNKNOWN;

    arm_info.version = 0.1;
    arm_info.name = string("Simulated Robot Arm");

    joint_info.push_back(createJointInfo(0, ROTATION, 0, 90, 65, 0, -180, 180));
    joint_data.push_back(createJointData(0, 0));

    joint_info.push_back(createJointInfo(1, ROTATION, 90, 0, 0, 90, 0, 180));
    joint_data.push_back(createJointData(1, 90));

    joint_info.push_back(createJointInfo(2, ROTATION, -90, 0, 0, 132, -110, 110));
    joint_data.push_back(createJointData(2, -90));

    joint_info.push_back(createJointInfo(3, ROTATION, 0, 0, 0, 60, -90, 90));
    joint_data.push_back(createJointData(3, 0));

    joint_info.push_back(createJointInfo(4, FIXED, 90, 90, 0, 0, 0, 0));
    joint_data.push_back(createJointData(4, 0));

    joint_info.push_back(createJointInfo(5, ROTATION, 0, 0, 20, 0, -90, 90));
    joint_data.push_back(createJointData(5, 0));

    joint_info.push_back(createJointInfo(6, GRIPPER, 0, 0, 0, 0, 0, 1));
    joint_data.push_back(createJointData(6, 0));

    arm_info.joints = joint_info.size();
}

SimulatedRobotArm::~SimulatedRobotArm() {


}

bool SimulatedRobotArm::isConnected() {

    return arm_data.state != UNKNOWN;

}

int SimulatedRobotArm::connect() {

    arm_data.state = CONNECTED;

	return 0;

}

int SimulatedRobotArm::disconnect() {

    arm_data.state = UNKNOWN;

	return 0;

}

#define MOVE_RESOLUTION_GRIP 0.1
#define MOVE_RESOLUTION_TRANSLATION 1
#define MOVE_RESOLUTION_ROTATION 5

bool SimulatedRobotArm::poll() {

    if (!isConnected()) return false;

    for (int i = 0; i < joint_data.size(); i++) {
        float resolution = joint_info[i].type == ROTATION ?
            MOVE_RESOLUTION_ROTATION : (joint_info[i].type == TRANSLATION ? MOVE_RESOLUTION_TRANSLATION : MOVE_RESOLUTION_GRIP);

        if (joint_data[i].position - joint_data[i].position_goal >= resolution / 2)
            joint_data[i].position -= resolution;
        else if (joint_data[i].position - joint_data[i].position_goal < -resolution / 2)
            joint_data[i].position += resolution;
        else
            joint_data[i].position = joint_data[i].position_goal;

    }

    return true;
}

int SimulatedRobotArm::calibrate(int joint) {
    return 0;
}

int SimulatedRobotArm::startControl() {
    arm_data.state = ACTIVE;

    return 0;
}

int SimulatedRobotArm::stopControl() {
    arm_data.state = PASSIVE;

    for (int i = 0; i < joint_data.size(); i++) {
        joint_data[i].position_goal = joint_data[i].position;
    }

	return 0;
}

int SimulatedRobotArm::lock(int joint) {
    return 0;
}

int SimulatedRobotArm::release(int joint) {
	return 0;
}

int SimulatedRobotArm::rest() {
	return 0;
}

int SimulatedRobotArm::size() {

    return joint_data.size();

}

int SimulatedRobotArm::moveTo(int joint, float speed, float position) {

    if (joint < 0 || joint >= joint_data.size())
        return -1;

    if (arm_data.state != ACTIVE)
        return -2;

    if (joint_info[joint].position_min > position)
        position = joint_info[joint].position_min;

    if (joint_info[joint].position_max < position)
        position = joint_info[joint].position_max;

    joint_data[joint].position_goal = position;

	return 0;
}

int SimulatedRobotArm::move(int joint, float speed) {

    if (joint < 0 || joint >= joint_data.size())
        return -1;

    if (arm_data.state != ACTIVE)
        return -2;

    float position = joint_data[joint].position_goal;

    if (speed < 0)
        position = joint_info[joint].position_min;

    if (speed > 0)
        position = joint_info[joint].position_max;

    joint_data[joint].position_goal = position;

	return 0;
}

int SimulatedRobotArm::getArmInfo(ArmInfo &data) {

    data = arm_info;

    return 0;

}

int SimulatedRobotArm::getArmData(ArmData &data) {

    data = arm_data;

	return 0;
}

int SimulatedRobotArm::getJointInfo(int joint, JointInfo &data) {

    if (joint < 0 || joint >= joint_info.size())
        return -1;

    data = joint_info[joint];

	return 0;
}

int SimulatedRobotArm::getJointData(int joint, JointData &data) {

    if (joint < 0 || joint >= joint_data.size())
        return -1;

    data = joint_data[joint];

	if (joint_info[joint].type == ::FIXED) {
		data.dh_position = data.position;
		data.dh_goal = data.position_goal;
	} else {
	    data.dh_position = (((data.position - joint_info[joint].position_min) / (joint_info[joint].position_max - joint_info[joint].position_min)) * (joint_info[joint].dh_max - joint_info[joint].dh_min)) + joint_info[joint].dh_min;
	    data.dh_goal = (((data.position_goal - joint_info[joint].position_min) / (joint_info[joint].position_max - joint_info[joint].position_min)) * (joint_info[joint].dh_max - joint_info[joint].dh_min)) + joint_info[joint].dh_min;
	}
	return true;
}


