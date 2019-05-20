#ifndef __OPENSERVO_MANIPULATOR_H
#define __OPENSERVO_MANIPULATOR_H

#include <string>
#include <vector>
#include <queue>

#include <openservo.h>

#include "manipulator.h"

typedef struct MotorData {
	int servo_id;
	int joint_id;
	int AD_min;
	int AD_max;
	int AD_center;
	float factor;
} MotorData;

class OpenServoManipulator : public Manipulator
{
public:
	OpenServoManipulator(const string& device,
    const string& model_file, const string& bind_file);
	~OpenServoManipulator();

	virtual int size();
	virtual bool move(int joint, float position, float speed);

	virtual ManipulatorDescription describe();
	virtual ManipulatorState state();

	bool process();

private:

	virtual int load_description(const string& model_file, const string& bind_file);

	struct ServoRuntimeData
	{
		int address;
		std::deque<int> position_median;
		std::deque<int> goal_median;
	};

  std::vector<ServoRuntimeData> runtime_data;

  ManipulatorDescription _description;
  ManipulatorState _state;
  std::vector<MotorData> servos;
  int read_rate;

  openservo::ServoBus bus;

  int joint_to_motor(int joint);
  int motor_to_joint(int motor);

};

#endif

/*
    Joint description:
      - min, max AD values -> dh_min in dh_max calculated based on parameters
      - origin value for AD
      - max angle
      - conversion factor from AD to angle
      - dh_theta, dh_alpha, dh_d, dh_a, dh_min (computed later), dh_max(computed later)
*/
