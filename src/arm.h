
#ifndef __ROBOT_ARM_H
#define __ROBOT_ARM_H

#include <string>
#include <vector>

using namespace std;

typedef struct ArmInfo {
    unsigned int id;
    float version;
    int joints; 
    int update_rate;
    int update_rate_max;
    string name;
} ArmInfo;
	
enum ArmState {UNKNOWN = 0, CONNECTED, PASSIVE, ACTIVE, CALIBRATION };

typedef struct ArmData {
	unsigned int id;
	ArmState state; 
	float input_voltage;
	float motors_voltage;
	float motors_current;
	int error_id;
} ArmData;

enum JointType {ROTATION = 0, TRANSLATION, GRIPPER_DUAL};
	
typedef struct JointInfo {
	unsigned int id;
	int joint_id;
    JointType type;
    float dh_theta;
    float dh_alpha;
    float dh_d;
    float dh_r;
	float position_min;		// [°]
	float position_max;		// [°]
	float limit_voltage;	// [V]
	float limit_current;	// [A]
} JointInfo;


typedef struct JointData {
	unsigned int id;
	int joint_id;
	float position;			// [°]
	float position_goal;	// [°]
	int state_id;
	int error_id;
} JointData;

class RobotArm {
public:
    RobotArm() {};
    ~RobotArm() {};
	
    virtual int connect() = 0;
    virtual int disconnect() = 0;
	virtual int poll() = 0;

	virtual bool calibrate(int joint = -1) = 0;
	virtual int lock(int joint = -1) = 0;
	virtual int release(int joint = -1) = 0;
	virtual int rest() = 0;
	
    virtual int size() = 0;
	virtual int move(int joint, float speed, float position = 0) = 0;
	
	virtual bool getArmInfo(ArmInfo &data) = 0;
	virtual bool getArmData(ArmData &data) = 0;
	virtual bool getJointInfo(int joint, JointInfo &data) = 0;
	virtual bool getJointData(int joint, JointData &data) = 0;

};

string armStateToString(ArmState status);

string jointTypeToString(JointType type);

JointInfo createJointInfo(int id, JointType type, float dh_theta, float dh_alpha, float dh_d, float dh_a, float min, float max);

JointData createJointData(int id, float position);

#endif

