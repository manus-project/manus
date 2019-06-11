#ifndef __SIMULATED_H
#define __SIMULATED_H

#include "manipulator.h"

class SimulatedManipulator : public Manipulator {
public:
    SimulatedManipulator(const string& model, float speed);
    ~SimulatedManipulator();

	bool step(float time);
	
    virtual int size();
	virtual bool move(int joint, float speed, float position);	

	virtual ManipulatorDescription describe();
	virtual ManipulatorState state();

private:

	float speed;

    ManipulatorDescription _description;
    ManipulatorState _state;

};

#endif
