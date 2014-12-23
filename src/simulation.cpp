
#include "simulation.h"

SimulatedRobotArm::SimulatedRobotArm() {

    arm_data.state = UNKNOWN;

    arm_info.version = 0.1;
    arm_info.name = string("Simulated Robot Arm");

    joint_info.push_back(createJointInfo(0, ROTATION, 0, 90, 65, 0, -180, 180));
    joint_data.push_back(createJointData(0, 0));

    joint_info.push_back(createJointInfo(1, ROTATION, 120, 0, 0, 90, 0, 170));
    joint_data.push_back(createJointData(1, 120));

    joint_info.push_back(createJointInfo(2, ROTATION, -90, 0, 0, 132, -110, 110));
    joint_data.push_back(createJointData(2, -90));

    joint_info.push_back(createJointInfo(3, ROTATION, 0, 0, 0, 70, -70, 70));
    joint_data.push_back(createJointData(3, -90));

    joint_info.push_back(createJointInfo(4, GRIPPER, 0, -90, 0, 60, 0, 1));
    joint_data.push_back(createJointData(4, 0));

    arm_info.joints = joint_info.size();
}

SimulatedRobotArm::~SimulatedRobotArm() {


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

int SimulatedRobotArm::poll() {

    for (int i = 0; i < joint_data.size(); i++) {
        int resolution = joint_info[i].type == ROTATION ? 
            MOVE_RESOLUTION_ROTATION : (joint_info[i].type == TRANSLATION ? MOVE_RESOLUTION_TRANSLATION : MOVE_RESOLUTION_GRIP);

        if (joint_data[i].position - joint_data[i].position_goal >= resolution / 2)
            joint_data[i].position -= resolution;

        if (joint_data[i].position - joint_data[i].position_goal <= -resolution / 2)
            joint_data[i].position += resolution;

    }

    return 0;
}

bool SimulatedRobotArm::calibrate(int joint) {
    return true;
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

int SimulatedRobotArm::move(int joint, float speed, float position) {

    if (joint < 0 || joint >= joint_data.size())
        return false;

    if (joint_info[joint].position_min > position)
        position = joint_info[joint].position_min;

    if (joint_info[joint].position_max < position)
        position = joint_info[joint].position_max;

    joint_data[joint].position_goal = position;

	return 0;
}

bool SimulatedRobotArm::getArmInfo(ArmInfo &data) {

    data = arm_info;

    return true;

}

bool SimulatedRobotArm::getArmData(ArmData &data) {

    data = arm_data;

	return true;
}

bool SimulatedRobotArm::getJointInfo(int joint, JointInfo &data) {

    if (joint < 0 || joint >= joint_info.size())
        return false;

    data = joint_info[joint];

	return true;
}

bool SimulatedRobotArm::getJointData(int joint, JointData &data) {

    if (joint < 0 || joint >= joint_data.size())
        return false;

    data = joint_data[joint];

	return true;
}


