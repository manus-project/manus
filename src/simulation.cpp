
#include <iostream>
#include <fstream>
#include <memory>
#include <cmath>

#include <echolib/client.h>
//#include <echolib/opencv.h>

using namespace std;
using namespace echolib;

#include "simulation.h"

SimulatedManipulator::SimulatedManipulator(const string& filename, float speed) : speed(speed) {

    if (!parse_description(filename, _description)) {
        throw ManipulatorException("Unable to parse manipulator model description");
    }

    for (int i = 0; i < _description.joints.size(); i++) {
        _state.joints.push_back(joint_state(_description.joints[i], _description.joints[i].dh_safe));
    }

}

SimulatedManipulator::~SimulatedManipulator() {


}

#define MOVE_RESOLUTION_GRIP 0.5
#define MOVE_RESOLUTION_TRANSLATION 30
#define MOVE_RESOLUTION_ROTATION (M_PI / 2)

bool SimulatedManipulator::step(float time) {

    bool idle = true;

    for (int i = 0; i < _description.joints.size(); i++) {
        float resolution = _description.joints[i].type == JOINTTYPE_ROTATION ?
            MOVE_RESOLUTION_ROTATION : (_description.joints[i].type == JOINTTYPE_TRANSLATION ? MOVE_RESOLUTION_TRANSLATION : MOVE_RESOLUTION_GRIP);

        resolution *= (time / 1000.0) * _state.joints[i].speed * speed;

        if ((_state.joints[i].position - _state.joints[i].goal) > (resolution / 2)) {
            _state.joints[i].position -= resolution;
            _state.joints[i].type = JOINTSTATETYPE_MOVING;
            idle = false;
        }
        else if ((_state.joints[i].position - _state.joints[i].goal) < (-resolution / 2)) {
            _state.joints[i].position += resolution;
            _state.joints[i].type = JOINTSTATETYPE_MOVING;
            idle = false;
        }
        else {
            _state.joints[i].position = _state.joints[i].goal;
            _state.joints[i].type = JOINTSTATETYPE_IDLE;
        }

    }

    _state.state = idle ? MANIPULATORSTATETYPE_PASSIVE : MANIPULATORSTATETYPE_ACTIVE;

    return idle;
}

int SimulatedManipulator::size() {
    return _state.joints.size();
}

bool SimulatedManipulator::move(int joint, float position, float speed) {

    if (joint < 0 || joint >= size())
        return false;

    if (_description.joints[joint].dh_min > position)
        position = _description.joints[joint].dh_min;

    if (_description.joints[joint].dh_max < position)
        position = _description.joints[joint].dh_max;

    _state.joints[joint].goal = position;
    _state.joints[joint].speed = speed;

	return true;
}

ManipulatorDescription SimulatedManipulator::describe() {

    return _description;

}

ManipulatorState SimulatedManipulator::state() {

    return _state;

}

#define SIMULATION_DELTA 50

int main(int argc, char** argv) {

    if (argc < 2) {
        cerr << "Missing manipulator description file path." << endl;
        return -1;
    }

    shared_ptr<SimulatedManipulator> manipulator = shared_ptr<SimulatedManipulator>(new SimulatedManipulator(string(argv[1]), 1));

    SharedClient client = echolib::connect();

    ManipulatorManager manager(client, manipulator);

    while (echolib::wait(SIMULATION_DELTA)) {        
        manager.update();
        manipulator->step(SIMULATION_DELTA);
    }

    exit(0);
}


