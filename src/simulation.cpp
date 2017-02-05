
#include <iostream>
#include <fstream>
#include <memory>
#include <cmath>

#include <echolib/client.h>
//#include <echolib/opencv.h>

using namespace std;
using namespace echolib;

#include "simulation.h"

SimulatedManipulator::SimulatedManipulator() {

    _description.version = 0.1;
    _description.name = string("Simulated Robot Arm");

    _description.joints.push_back(joint_description(::ROTATION, 0, 90, 65, 0, -180, 180));
    _state.joints.push_back(joint_state(_description.joints[_description.joints.size()-1],0));

    _description.joints.push_back(joint_description(::ROTATION, 90, 0, 0, 90, 0, 180));
    _state.joints.push_back(joint_state(_description.joints[_description.joints.size()-1],90));

    _description.joints.push_back(joint_description(::ROTATION, -90, 0, 0, 132, -110, 110));
    _state.joints.push_back(joint_state(_description.joints[_description.joints.size()-1],-90));

    _description.joints.push_back(joint_description(::ROTATION, 0, 0, 0, 60, -90, 90));
    _state.joints.push_back(joint_state(_description.joints[_description.joints.size()-1],0));

    _description.joints.push_back(joint_description(FIXED, 90, 90, 0, 0, 0, 0));
    _state.joints.push_back(joint_state(_description.joints[_description.joints.size()-1],0));

    _description.joints.push_back(joint_description(::ROTATION, 0, 0, 20, 0, -90, 90));
    _state.joints.push_back(joint_state(_description.joints[_description.joints.size()-1],0));

    _description.joints.push_back(joint_description(GRIPPER, 0, 0, 0, 0, 0, 1));
    _state.joints.push_back(joint_state(_description.joints[_description.joints.size()-1],0));

}

SimulatedManipulator::~SimulatedManipulator() {


}

#define MOVE_RESOLUTION_GRIP 0.5
#define MOVE_RESOLUTION_TRANSLATION 30
#define MOVE_RESOLUTION_ROTATION (M_PI / 2)

bool SimulatedManipulator::step(float time) {

    bool idle = true;

    for (int i = 0; i < _description.joints.size(); i++) {
        float resolution = _description.joints[i].type == ::ROTATION ?
            MOVE_RESOLUTION_ROTATION : (_description.joints[i].type == ::TRANSLATION ? MOVE_RESOLUTION_TRANSLATION : MOVE_RESOLUTION_GRIP);

        resolution *= (time / 1000.0) * _state.joints[i].speed;

        if ((_state.joints[i].position - _state.joints[i].goal) > (resolution / 2)) {
            _state.joints[i].position -= resolution;
            _state.joints[i].type = MOVING;
            idle = false;
        }
        else if ((_state.joints[i].position - _state.joints[i].goal) < (-resolution / 2)) {
            _state.joints[i].position += resolution;
            _state.joints[i].type = MOVING;
            idle = false;
        }
        else {
            _state.joints[i].position = _state.joints[i].goal;
            _state.joints[i].type = IDLE;
        }

    }

    _state.state = idle ? PASSIVE : ACTIVE;

    return idle;
}

int SimulatedManipulator::lock(int joint) {
    return 0;
}

int SimulatedManipulator::release(int joint) {
	return 0;
}

int SimulatedManipulator::rest() {

    for (int i = 0; i < _state.joints.size(); i++) {
        _state.joints[i].goal = _state.joints[i].position;
    }

	return 0;
}

int SimulatedManipulator::size() {
    return _state.joints.size();
}

int SimulatedManipulator::move(int joint, float position, float speed) {

    if (joint < 0 || joint >= size())
        return -1;

    if (_description.joints[joint].dh_min > position)
        position = _description.joints[joint].dh_min;

    if (_description.joints[joint].dh_max < position)
        position = _description.joints[joint].dh_max;

    _state.joints[joint].goal = position;
    _state.joints[joint].speed = speed;

	return 0;
}

ManipulatorDescription SimulatedManipulator::describe() {

    return _description;

}


ManipulatorState SimulatedManipulator::state() {

    return _state;

}

int main(int argc, char** argv) {

    SharedClient client = echolib::connect();

    shared_ptr<SimulatedManipulator> manipulator = shared_ptr<SimulatedManipulator>(new SimulatedManipulator());

    ManipulatorManager manager(client, manipulator);

    while (echolib::wait(50)) {
        manipulator->step(50);
        manager.update();
    }

    exit(0);
}


