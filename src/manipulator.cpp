
#include <iostream>
#include <memory>
#include <functional>
#include <cmath>

#include "manipulator.h"
#include <yaml-cpp/yaml.h>

using namespace std;

ManipulatorException::ManipulatorException(char const* const message) throw()
    : std::runtime_error(message) {

}

char const * ManipulatorException::what() const throw() {
    return exception::what();
}

string manipulator_state_string(ManipulatorStateType status) {

    switch (status) {
    case MANIPULATORSTATETYPE_UNKNOWN: return "unknown";
    case MANIPULATORSTATETYPE_CONNECTED: return "connected";
    case MANIPULATORSTATETYPE_PASSIVE: return "passive";
    case MANIPULATORSTATETYPE_ACTIVE: return "active";
    case MANIPULATORSTATETYPE_CALIBRATION: return "calibration";
    }

    return "unknown";
}

string joint_type_string(JointType type) {

    switch (type) {
    case JOINTTYPE_ROTATION: return "rotation";
    case JOINTTYPE_TRANSLATION: return "translation";
    case JOINTTYPE_FIXED: return "fixed";
    case JOINTTYPE_GRIPPER: return "gripper";
    }

    return "unknown";
}

bool parse_joint (const YAML::Node& node, JointDescription& joint) {
    string type = node["type"].as<string>();
    if (type == "rotation") {
        joint.type = JOINTTYPE_ROTATION;
        joint.dh_alpha = DEGREE_TO_RADIAN(node["dh"]["alpha"].as<float>());
        joint.dh_d = node["dh"]["d"].as<float>();
        joint.dh_a = node["dh"]["a"].as<float>();
        joint.dh_min = DEGREE_TO_RADIAN(node["min"].as<float>());
        joint.dh_max = DEGREE_TO_RADIAN(node["max"].as<float>());
        joint.dh_safe = DEGREE_TO_RADIAN(node["safe"].as<float>());

        joint.dh_theta = joint.dh_safe;
        return true;
    }
    if (type == "translation") {
        joint.type = JOINTTYPE_TRANSLATION;
        joint.dh_theta = DEGREE_TO_RADIAN(node["dh"]["theta"].as<float>());
        joint.dh_alpha = DEGREE_TO_RADIAN(node["dh"]["alpha"].as<float>());
        joint.dh_a = node["dh"]["a"].as<float>();
        joint.dh_min = node["min"].as<float>();
        joint.dh_max = node["max"].as<float>();
        joint.dh_safe = node["safe"].as<float>();

        joint.dh_d = joint.dh_safe;
        return true;
    }
    if (type == "fixed") {
        joint.type = JOINTTYPE_FIXED;
        joint.dh_theta = DEGREE_TO_RADIAN(node["dh"]["theta"].as<float>());
        joint.dh_alpha = DEGREE_TO_RADIAN(node["dh"]["alpha"].as<float>());
        joint.dh_d = node["dh"]["d"].as<float>();
        joint.dh_a = node["dh"]["a"].as<float>();
        joint.dh_min = 0;
        joint.dh_max = 0;
        joint.dh_safe = 0;
        return true;
    }
    if (type == "gripper") {
        joint.type = JOINTTYPE_GRIPPER;
        joint.dh_theta = 0;
        joint.dh_alpha = 0;
        joint.dh_d = node["grip"].as<float>();
        joint.dh_a = 0;
        joint.dh_min = node["min"].as<float>();
        joint.dh_max = node["max"].as<float>();
        joint.dh_safe = 0;
        return true;
    }
    return false;
}

bool parse_description(const string& filename, ManipulatorDescription& manipulator) {

    YAML::Node doc = YAML::LoadFile(filename);

    manipulator.name = doc["name"].as<string>();
    manipulator.version = doc["version"].as<float>();

    const YAML::Node& joints = doc["joints"];
    manipulator.joints.clear();

    for (int i = 0; i < joints.size(); i++) {
        JointDescription d;
        parse_joint(joints[i], d);
        manipulator.joints.push_back(d);
    }

    return true;
}

JointDescription joint_description(JointType type, float dh_theta, float dh_alpha, float dh_d, float dh_a, float min, float max) {
    JointDescription joint;
    joint.type = type;
    joint.dh_theta = DEGREE_TO_RADIAN(dh_theta);
    joint.dh_alpha = DEGREE_TO_RADIAN(dh_alpha);
    joint.dh_d = dh_d;
    joint.dh_a = dh_a;
    joint.dh_min = type == JOINTTYPE_ROTATION ? DEGREE_TO_RADIAN(min) : min;
    joint.dh_max = type == JOINTTYPE_ROTATION ? DEGREE_TO_RADIAN(max) : max;
    return joint;
}

JointState joint_state(const JointDescription& joint, float position, JointStateType type) {
    JointState state;
    state.type = type;
    state.position = joint.type == JOINTTYPE_ROTATION ? DEGREE_TO_RADIAN(position) : position;
    state.goal = joint.type == JOINTTYPE_ROTATION ? DEGREE_TO_RADIAN(position) : position;
    return state;
}

bool close_enough(float a, float b) {
    return std::fabs(a - b) < 0.05;
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
        // Move to safe position if no joint given
        if (t->segments[s].joints.size() == 0) {
            cout << "Moving to safe position" << endl;
            for (size_t j = 0; j < manipulator->size(); j++) {
                JointCommand tmp;
                tmp.goal = description.joints[j].dh_safe;
                tmp.speed = 1;
                t->segments[s].joints.push_back(tmp);
            }
        }

        if (t->segments[s].joints.size() != manipulator->size()) {
            cout << "Wrong manipulator size " << t->segments[s].joints.size() << " vs. " << manipulator->size() << endl;
            return;
        }
        for (size_t j = 0; j < manipulator->size(); j++) {
            float goal = t->segments[s].joints[j].goal;
            if (description.joints[j].type == JOINTTYPE_ROTATION) {
                goal = normalizeAngle(goal, description.joints[j].dh_min, description.joints[j].dh_max);
            }

            if (goal < description.joints[j].dh_min ||
                    t->segments[s].joints[j].goal > description.joints[j].dh_max) {
                if (description.joints[j].type != JOINTTYPE_FIXED) {
                    cout << "Warning: wrong joint " << j << " goal " << goal << " out of range " << description.joints[j].dh_min << " to " << description.joints[j].dh_max << ". Truncating." << endl;
                    goal = max(description.joints[j].dh_min, min(description.joints[j].dh_max, goal));
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
    state.type = PLANSTATETYPE_RUNNING;
    planstate_publisher->send(state);

    plan = t;

    step(true);

}

void ManipulatorManager::update() {

    if (manipulator->state().state != MANIPULATORSTATETYPE_UNKNOWN && !description_publisher) {
        cout << "Manipulator ready" << endl;
        ManipulatorDescription description = manipulator->describe();
        description_publisher = make_shared<StaticPublisher<ManipulatorDescription> >(client, "description", description);
    }

    if (manipulator->state().state != MANIPULATORSTATETYPE_PASSIVE && manipulator->state().state != MANIPULATORSTATETYPE_UNKNOWN) {
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

    bool idle = true;
    bool goal = true;

    ManipulatorState state = manipulator->state();
    for (size_t i = 0; i < manipulator->size(); i++) {
        idle &= state.joints[i].type == JOINTSTATETYPE_IDLE;
        goal &= close_enough(state.joints[i].position, state.joints[i].goal);
    }

    if (goal || force) {

        if (plan->segments.size() == 0) {
            PlanState state;
            state.identifier = plan->identifier;
            state.type = PLANSTATETYPE_COMPLETED;
            planstate_publisher->send(state);
            plan.reset();
            return;
        }

        for (size_t i = 0; i < manipulator->size(); i++) {
            manipulator->move(i, plan->segments[0].joints[i].goal, plan->segments[0].joints[i].speed);
        }

        plan->segments.erase(plan->segments.begin());
    }

}

