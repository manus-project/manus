
#ifndef __SIMULATED_ROBOT_ARM_H
#define __SIMULATED_ROBOT_ARM_H

#include "arm.h"

class SimulatedRobotArm : public RobotArm {
public:
    SimulatedRobotArm();
    ~SimulatedRobotArm();

	virtual bool isConnected();
	
    virtual int connect();
    virtual int disconnect();
	virtual bool poll();

	virtual int calibrate(int joint = -1);
	virtual int startControl();
	virtual int stopControl();
	virtual int lock(int joint = -1);
	virtual int release(int joint = -1);
	virtual int rest();
	
    virtual int size();
	virtual int move(int joint, float speed);
	virtual int moveTo(int joint, float speed, float position);	

	virtual int getArmInfo(ArmInfo &data);
	virtual int getArmData(ArmData &data);
	virtual int getJointInfo(int joint, JointInfo &data);
	virtual int getJointData(int joint, JointData &data);

private:

    ArmData arm_data;
    ArmInfo arm_info;

    vector<JointInfo> joint_info;
    vector<JointData> joint_data;

};

#endif
