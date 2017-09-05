#ifndef __OPENSERVOROBOT_MANIPULATOR_H
#define __OPENSERVOROBOT_MANIPULATOR_H

#include <string>
#include <vector>
#include <queue>

#include "manipulator.h"

//#include <openservolib/openservo_lib.h>

//using namespace std;

typedef struct sv_info
{
  int servo_id;
  int joint_id;
  int AD_min;
  int AD_max;
  int AD_center;
  float factor;
} servo_info;


class OpenServoRobot : public Manipulator, public IOBase
{
public:
  //OpenServoRobot(string path_to_i2c_port, string path_to_robot_description_file);
  OpenServoRobot(string path_to_i2c_port, const string& modelfile, const string& calibfile);
  ~OpenServoRobot();

  virtual int lock(int joint = -1);
  virtual int release(int joint = -1);
  virtual int rest();

  virtual int size();
  virtual int move(int joint, float position, float speed);

  virtual ManipulatorDescription describe();
  virtual ManipulatorState state();

  virtual int get_file_descriptor();
	virtual bool handle_output();
	virtual bool handle_input();
	virtual void disconnect();

private:

  virtual int connectTo(string path_to_i2c_port);
  virtual int loadDescription(const string& modelfile, const string& calibfile);

  enum ActionType {MOVE, UPDATE_JOINTS};
  struct buff_data
  {
    ActionType action_type;
    int joint;
    float speed;
    float position;
  };

  struct servo_data
  {
    int address;
    std::deque<int> position_median;
    std::deque<int> goal_median;
  };

  // thread
  pthread_t thread1, thread_req;
  bool non_blocking;
  pthread_mutex_t q_mutex        = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_t sleep_mutex    = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_t read_servo_mutex    = PTHREAD_MUTEX_INITIALIZER;
  pthread_cond_t wake_up_condition = PTHREAD_COND_INITIALIZER;
  bool in_sleep;
  bool end_thread;
  std::queue<buff_data> q_out;

  std::vector<servo_data> runtime_data; 

  ManipulatorDescription _description;
  ManipulatorState _state;
  std::vector<servo_info> servos;
  int read_rate;

  OpenServo open_servo;

  static void* startRoutine(void* arg)
  {
    OpenServoRobot* ops = reinterpret_cast<OpenServoRobot*>(arg);
    ops->threadRoutine();
  }
  static void* startRoutineReq(void* arg)
  {
    OpenServoRobot* ops = reinterpret_cast<OpenServoRobot*>(arg);
    ops->threadRoutineReq();
  }
  void threadRoutine();
  void threadRoutineReq();

  void sendMove(int joint, float speed, float position);
  void updateJoints();
  int joint_to_motor(int joint);
  int motor_to_joint(int m);

  void printServos();

};

#endif

/*
    Joint description:
      - min, max AD vrednosti -> dh_min in dh_max se porečunata glede na parametre
      - izhodišno verdnost za AD
      - max kot servota
      - faktor pretvorbe iz AD v kot
      - dh_theta, dh_alpha, dh_d, dh_a, dh_min(se lahko naknadno poračuna), dh_max(se lahko naknadno poračuna)

*/
