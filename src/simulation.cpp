
#include "simulation.h"

SimulatedRobotArm::SimulatedRobotArm() {

    arm_data.state = UNKNOWN;

    arm_info.version = 0.1;
    arm_info.name = string("Simulated Robot Arm");

    joint_info.push_back(createJointInfo(0, ROTATION, 0, 90, 65, 0, -180, 180));
    joint_data.push_back(createJointData(0, 0));

    joint_info.push_back(createJointInfo(1, ROTATION, 120, 0, 0, 90, 0, 170));
    joint_data.push_back(createJointData(1, 120));

    joint_info.push_back(createJointInfo(2, ROTATION, -90, 0, 0, 132, -90, 90));
    joint_data.push_back(createJointData(2, -90));

    joint_info.push_back(createJointInfo(3, ROTATION, -90, 0, 0, 70, -90, 90));
    joint_data.push_back(createJointData(3, -90));

  //  joint_info.push_back(createJointInfo(3, GRIPPER_DUAL, 0, 0, 0, 0, 0, 1));
  //  joint_data.push_back(createJointData(3, 0));

    arm_info.joints = joint_info.size();
}

SimulatedRobotArm::~SimulatedRobotArm() {


}

int SimulatedRobotArm::connect() {

    arm_data.state = CONNECTED;

}

int SimulatedRobotArm::disconnect() {

    arm_data.state = UNKNOWN;

}

#define MOVE_RESOLUTION 5

int SimulatedRobotArm::poll() {

    for (int i = 0; i < joint_data.size(); i++) {
        if (joint_data[i].position - joint_data[i].position_goal >= MOVE_RESOLUTION / 2)
            joint_data[i].position -= MOVE_RESOLUTION;

        if (joint_data[i].position - joint_data[i].position_goal <= -MOVE_RESOLUTION / 2)
            joint_data[i].position += MOVE_RESOLUTION;

    }

    return 0;
}

bool SimulatedRobotArm::calibrate(int joint) {
    return true;
}

int SimulatedRobotArm::lock(int joint) {
    
}

int SimulatedRobotArm::release(int joint) {

}

int SimulatedRobotArm::rest() {

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

}

bool SimulatedRobotArm::getArmInfo(ArmInfo &data) {

    data = arm_info;

    return true;

}

bool SimulatedRobotArm::getArmData(ArmData &data) {

    data = arm_data;

}

bool SimulatedRobotArm::getJointInfo(int joint, JointInfo &data) {

    if (joint < 0 || joint >= joint_info.size())
        return false;

    data = joint_info[joint];
}

bool SimulatedRobotArm::getJointData(int joint, JointData &data) {

    if (joint < 0 || joint >= joint_data.size())
        return false;

    data = joint_data[joint];
}


