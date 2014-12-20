
#ifndef __SIMULATED_ROBOT_ARM_H
#define __SIMULATED_ROBOT_ARM_H

#include "arm.h"

class SimulatedRobotArm : public RobotArm {
public:
    SimulatedRobotArm();
    ~SimulatedRobotArm();
	
    virtual int connect();
    virtual int disconnect();
	virtual int poll();

	virtual bool calibrate(int joint = -1);
	virtual int lock(int joint = -1);
	virtual int release(int joint = -1);
	virtual int rest();
	
    virtual int size();
	virtual int move(int joint, float speed, float position = 0);
	
	virtual bool getArmInfo(ArmInfo &data);
	virtual bool getArmData(ArmData &data);
	virtual bool getJointInfo(int joint, JointInfo &data);
	virtual bool getJointData(int joint, JointData &data);

private:

    ArmData arm_data;
    ArmInfo arm_info;

    vector<JointInfo> joint_info;
    vector<JointData> joint_data;

};

#endif
