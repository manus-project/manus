#ifndef __SIMULATED_H
#define __SIMULATED_H

#include "manipulator.h"

class SimulatedManipulator : public Manipulator {
public:
    SimulatedManipulator(const string& filename, float speed);
    ~SimulatedManipulator();

	bool step(float time);
	virtual int lock(int joint = -1);
	virtual int release(int joint = -1);
	virtual int rest();
	
    virtual int size();
	virtual int move(int joint, float speed, float position);	

	virtual ManipulatorDescription describe();
	virtual ManipulatorState state();

private:

	float speed;

    ManipulatorDescription _description;
    ManipulatorState _state;

};

#endif
