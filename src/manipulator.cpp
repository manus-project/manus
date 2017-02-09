
#include <iostream>
#include <memory>
#include <functional>
#include <cmath>

#include "manipulator.h"

using namespace std;

#define RADIAN_TO_DEGREE(X) ((X * 180) / M_PI )
#define DEGREE_TO_RADIAN(X) ((X / 180) * M_PI )

string manipulator_state_string(ManipulatorStateType status) {

    switch (status) {
    case UNKNOWN: return "unknown";
    case CONNECTED: return "connected";
    case PASSIVE: return "passive";
    case ACTIVE: return "active";
    case CALIBRATION: return "calibration";
    }

    return "unknown";
}

string joint_type_string(JointType type) {

    switch (type) {
    case ROTATION: return "rotation";
    case TRANSLATION: return "translation";
    case FIXED: return "fixed";
    case GRIPPER: return "gripper";
    }

    return "unknown";
}

JointDescription joint_description(JointType type, float dh_theta, float dh_alpha, float dh_d, float dh_a, float min, float max) {
    JointDescription joint;
    joint.type = type;
    joint.dh_theta = DEGREE_TO_RADIAN(dh_theta);
    joint.dh_alpha = DEGREE_TO_RADIAN(dh_alpha);
    joint.dh_d = dh_d;
    joint.dh_a = dh_a;
    joint.dh_min = type == JointType::ROTATION ? DEGREE_TO_RADIAN(min) : min;
    joint.dh_max = type == JointType::ROTATION ? DEGREE_TO_RADIAN(max) : max;
    return joint;
}

JointState joint_state(const JointDescription& joint, float position, JointStateType type) {
    JointState state;
    state.type = type;
    state.position = joint.type == JointType::ROTATION ? DEGREE_TO_RADIAN(position) : position;
    state.goal = joint.type == JointType::ROTATION ? DEGREE_TO_RADIAN(position) : position;
    return state;
}
/*
double normalizeAngleRadian(double val, double min, double max) {
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

double convertOutgoing(double value, const JointInfo& info) {

    if (info.type == ::ROTATION) {
        return (value / 180) * M_PI;
    }
    if (info.type == ::GRIPPER) {
        return (value - info.dh_min) / (info.dh_max - info.dh_min);
    }
    return value;
}

double convertIncoming(double value, const JointInfo& info) {

    if (info.type == ::ROTATION) {
        value = normalizeAngleRadian(value, info.dh_min, info.dh_max);
        return (value * 180) / M_PI;
    }
    if (info.type == ::GRIPPER) {
        return (value) * (info.dh_max - info.dh_min) + info.dh_min;
    }
    return value;
}*/

bool close_enough(float a, float b) {
    return std::fabs(a - b) < 0.1;
}

inline float normalizeAngle(float val, const float min, const float max) {

    if (val > max) {
        //Find actual angle offset
        float diffangle = fmod(val - max, 2 * M_PI);
        // Add that to upper bound and go back a full rotation
        val = max + diffangle - 2 * M_PI;
    }

    if (val < min) {
        //Find actual angle offset
        float diffangle = fmod(min - val, 2 * M_PI);
        // Add that to upper bound and go back a full rotation
        val = min - diffangle + 2 * M_PI;
    }
    return val;
}

ManipulatorManager::ManipulatorManager(SharedClient client, shared_ptr<Manipulator> manipulator): manipulator(manipulator), client(client),
    watcher(client, "state", std::bind(&ManipulatorManager::on_subscribers, this, std::placeholders::_1)), subscribers(0) {

    state_publisher = make_shared<TypedPublisher<ManipulatorState> >(client, "state");

    planstate_publisher = make_shared<TypedPublisher<PlanState> >(client, "planstate");

    plan_listener = make_shared<TypedSubscriber<Plan> >(client, "plan",
    [this](shared_ptr<Plan> param) {
        this->push(param);
    });

}

ManipulatorManager::~ManipulatorManager() {

    flush();

}

void ManipulatorManager::flush() {

    plan.reset();
/*
    ManipulatorState state = manipulator->state();

    for (size_t i = 0; i < manipulator->size(); i++) {
        manipulator->move(i, state.joints[i].position, 1);
    }
*/
}

void ManipulatorManager::push(shared_ptr<Plan> t) {

    ManipulatorDescription description = manipulator->describe();

    cout << "Received new plan" << endl;

    for (size_t s = 0; s < t->segments.size(); s++) {
        if (t->segments[s].joints.size() != manipulator->size()) {
            cout << "Wrong manipulator size " << t->segments[s].joints.size() << " vs. " << manipulator->size() << endl;
            return;
        }
        for (size_t j = 0; j < manipulator->size(); j++) {
            float goal = t->segments[s].joints[j].goal;
            if (description.joints[j].type == ROTATION) {
                goal = normalizeAngle(goal, description.joints[j].dh_min, description.joints[j].dh_max);
            }

            if (goal < description.joints[j].dh_min ||
                    t->segments[s].joints[j].goal > description.joints[j].dh_max) {
                if (description.joints[j].type != FIXED) {
                    cout << "Wrong joint " << j << " goal " << goal << " out of range " << description.joints[j].dh_min << " to " << description.joints[j].dh_max << endl;
                    return;
                } else {
                    goal = description.joints[j].dh_min;
                }
            }
            t->segments[s].joints[j].goal = goal;
        }
    }

    flush();

    PlanState state;
    state.identifier = t->identifier;
    state.type = RUNNING;
    planstate_publisher->send(state);

    plan = t;

    step(true);

}

void ManipulatorManager::update() {

    if (manipulator->state().state != UNKNOWN && !description_publisher) {
        ManipulatorDescription description = manipulator->describe();
        description_publisher = make_shared<StaticPublisher<ManipulatorDescription> >(client, "description", description);
    }

    if (manipulator->state().state != PASSIVE && manipulator->state().state != UNKNOWN) {
        state_publisher->send(manipulator->state());
    }

    step();
}

void ManipulatorManager::on_subscribers(int s) {
    if (s > subscribers) {
        state_publisher->send(manipulator->state());
    }
    subscribers = s;
}

void ManipulatorManager::step(bool force) {

    if (!plan) return;

    if (plan->segments.size() < 1) {
        PlanState state;
        state.identifier = plan->identifier;
        state.type = COMPLETED;
        planstate_publisher->send(state);
        plan.reset();
        return;
    }

    bool idle = true;
    bool goal = true;

    ManipulatorState state = manipulator->state();

    for (size_t i = 0; i < manipulator->size(); i++) {
        idle &= state.joints[i].type == IDLE;
        goal &= close_enough(state.joints[i].position, state.joints[i].goal);
        //cout << i << ": " << idle << " " << goal << " - " << state.joints[i].position << " "  << state.joints[i].goal << endl;
    }

    if (goal || force) {

        for (size_t i = 0; i < manipulator->size(); i++) {
            manipulator->move(i, plan->segments[0].joints[i].goal, plan->segments[0].joints[i].speed);
        }

        plan->segments.erase(plan->segments.begin());
    }

}

    