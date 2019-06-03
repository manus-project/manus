#include <cmath>
#include <chrono>
#include <numeric>
#include <unistd.h>

#include "openservorobot.h"

#include <yaml-cpp/yaml.h>

#define MEDIAN_WINDOW 20

using namespace openservo;

float scale_servo_to_joint(const MotorData& si, float ad_pos) {

  float pos_deg = (ad_pos - si.AD_center) * si.factor;
  return round(pos_deg * 100.0) / 100.0;

}

float scale_joint_to_servo(const MotorData& si, float pos) {

  float pos_ad = (pos / si.factor) + si.AD_center;
  return pos_ad;

}

OpenServoManipulator::OpenServoManipulator(const string& device,
    const string& model_file, const string& bind_file) {

  read_rate = 30;
  _state.state = MANIPULATORSTATETYPE_UNKNOWN;
  _description.name = "i2c Robot Manipulator";
  _description.version = 0.2f;

  if (bus.open(device))
    _state.state = MANIPULATORSTATETYPE_ACTIVE;

  int num = bus.scan();

  load_description(model_file, bind_file);

  if (num < servos.size())
    throw ManipulatorException("Not enough servos detected.");

}

OpenServoManipulator::~OpenServoManipulator() {

}

bool parse_bindings(const string& filename, ManipulatorDescription& manipulator, vector<MotorData>& servos) {

    YAML::Node doc = YAML::LoadFile(filename);
/*
    const YAML::Node& offset = doc["offset"];

    manipulator.frame.origin.x = offset["origin"]["x"].as<float>();
    manipulator.frame.origin.y = offset["origin"]["y"].as<float>();
    manipulator.frame.origin.z = offset["origin"]["z"].as<float>();

    manipulator.frame.rotation.x = offset["rotation"]["x"].as<float>();
    manipulator.frame.rotation.y = offset["rotation"]["y"].as<float>();
    manipulator.frame.rotation.z = offset["rotation"]["z"].as<float>();
*/
    const YAML::Node& joints = doc["motors"];

    servos.clear();

    for (int i = 0; i < joints.size(); i++) {

        if (manipulator.joints.size() <= i) return false;

        const YAML::Node& servo = joints[i];

        if (servo.IsDefined()) {
          MotorData d;

          d.servo_id = servo["id"].as<int>();
          d.joint_id = -1;
          d.AD_min = servo["min"].as<int>();
          d.AD_max = servo["max"].as<int>();
          d.AD_center = servo["center"].as<int>();
          d.factor = servo["factor"].as<float>();

          servos.push_back(d);
        }

    }

    return true;
}


int OpenServoManipulator::load_description(const string& modelfile, const string& bindfile) {

  if (!parse_description(modelfile, _description)) {
    throw ManipulatorException("Unable to parse manipulator model description");
  }

  if (!parse_bindings(bindfile, _description, servos)) {
    throw ManipulatorException("Unable to parse manipulator bindings description");
  }

  _state.joints.resize(_description.joints.size());
  int j = 0;
  for (int i = 0; i < _description.joints.size(); i++) {
    _state.joints[i].type = JOINTSTATETYPE_IDLE;

    if (_description.joints[i].type == JOINTTYPE_FIXED)
      continue;

    if (j >= servos.size())
      throw ManipulatorException("Not enough motors in calibration data.");

    runtime_data.resize(runtime_data.size() + 1);
    runtime_data[runtime_data.size() - 1].address = servos[j].servo_id;
    servos[j].joint_id = i;
    _description.joints[i].dh_min = scale_servo_to_joint(servos[j], servos[j].AD_min);
    _description.joints[i].dh_max = scale_servo_to_joint(servos[j], servos[j].AD_max);

    cout << "Verifying min-max data." << endl;
    ServoHandler sv = bus.find(servos[j].servo_id);

    if (sv->getMinSeek() != servos[j].AD_min || sv->getMaxSeek() != servos[j].AD_max) {

      cout << "Detected incorrect parameters, writing min-max data to motor " << i << endl;
      sv->unlock();
      sv->set("seek.max", servos[j].AD_max);
      sv->set("seek.min", servos[j].AD_min);
      bus.update();
      sv->registersCommit();
    }

    j++;
  }

  cout << "Joints: " << _description.joints.size() << " Motors: " << runtime_data.size() << endl;

  if (j != servos.size())
    throw ManipulatorException("Unassigned motors remaining.");

  return 1;
}

int OpenServoManipulator::size() {

  return _description.joints.size();

}

bool OpenServoManipulator::move(int joint, float position, float speed) {

  int motor = joint_to_motor(joint);

  if (motor < 0)
    return false;

  ServoHandler sv = bus.find(runtime_data[motor].address);

  if (!sv)
    return false;

  float pos = ::round(scale_joint_to_servo(servos[motor], position));

  sv->setSeekPosition((int)pos);

  runtime_data[motor].goal_median.clear();
  runtime_data[motor].goal_median.assign(MEDIAN_WINDOW, (int)pos);

  _state.joints[joint].goal = position;

  return true;

}

#define MEDIAN_WINDOW 20

ManipulatorDescription OpenServoManipulator::describe() {

  return _description;

}

ManipulatorState OpenServoManipulator::state() {

  return _state;

}

int OpenServoManipulator::joint_to_motor(int j) {

  if (j < 0 || j >= _description.joints.size()) return -1;

  if (_description.joints[j].type == JOINTTYPE_FIXED) return -1;

  int m = 0;
  for (int i = 0; i < j; i++) {
    if (_description.joints[i].type != JOINTTYPE_FIXED)
      m++;
  }

  return m;

}

int OpenServoManipulator::motor_to_joint(int m) {

  int mt = m;
  int j = 0;
  while (m > 0) {
    j++; m--;
    if (_description.joints[j].type == JOINTTYPE_FIXED)
      j++;
  }

  return j;

}

bool OpenServoManipulator::process() {

  if (!bus.update()) {
    cout << bus.getLastError() << endl;
    //return false;
  }

  // refresh state data
  for (int q = 0; q < _description.joints.size(); q++) {

    int motor = joint_to_motor(q);

    if (motor < 0) continue;

    ServoHandler sv = bus.find(runtime_data[motor].address);

    if (!sv) continue;

    runtime_data[motor].position_median.push_back(sv->getPosition());
    runtime_data[motor].goal_median.push_back(sv->getSeekPosition());

    if (runtime_data[motor].position_median.size() > MEDIAN_WINDOW)
      runtime_data[motor].position_median.pop_front();

    if (runtime_data[motor].goal_median.size() > MEDIAN_WINDOW)
      runtime_data[motor].goal_median.pop_front();

    vector<int> sorted_position(runtime_data[motor].position_median.begin(),
      runtime_data[motor].position_median.end());
    vector<int> sorted_goal(runtime_data[motor].goal_median.begin(),
      runtime_data[motor].goal_median.end());

    std::nth_element(sorted_position.begin(),
      sorted_position.begin() + sorted_position.size() / 2, sorted_position.end());

    float position = sorted_position[sorted_position.size() / 2];

    std::nth_element(sorted_goal.begin(),
      sorted_goal.begin() + sorted_goal.size() / 2, sorted_goal.end());
    float goal = sorted_goal[sorted_goal.size() / 2];

    _state.joints[q].position = scale_servo_to_joint(servos[motor], position);
    _state.joints[q].goal = scale_servo_to_joint(servos[motor], goal);
    _state.joints[q].speed = 1;

  }

  return true;

}

#define REFRESH_DELTA 20

using namespace std::chrono;

int main(int argc, char** argv) {

  if (argc < 3) {
    cerr << "Missing manipulator description and binding file paths." << endl;
    return -1;
  }

  string device;

  if (argc > 3) {
    device = string(argv[3]);
  }

  cout << "Starting OpenServo manipulator" << endl;

  try {

    shared_ptr<OpenServoManipulator> manipulator =
      shared_ptr<OpenServoManipulator>(new OpenServoManipulator(
                                         device, string(argv[1]), string(argv[2])));

    SharedClient client = echolib::connect(string(), "manipulator");
    ManipulatorManager manager(client, manipulator);

    int duration = 0;

    while (true) {
      if (!echolib::wait(std::max(1, 20 - duration))) break;

      steady_clock::time_point start = steady_clock::now();

      manager.update();
      if (!manipulator->process()) break;

      duration = duration_cast<milliseconds>(steady_clock::now() - start).count();

    }

  } catch (ManipulatorException &e) {
     cout << "Exception: " << e.what() << endl;

     exit(1);
  }


  exit(0);
}
