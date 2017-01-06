namespace manus.manipulator;

enumerate ManipulatorStateType { UNKNOWN, CONNECTED, PASSIVE, ACTIVE, CALIBRATION }
enumerate JointType { ROTATION, TRANSLATION, GRIPPER, FIXED }
enumerate JointStateType { IDLE, MOVING, ERROR }

structure Point {
	float x;
	float y;
	float z;
}

structure Rotation {
	float x;
	float y;
	float z;
}

structure JointDescription {
	JointType type;
	float dh_theta;
	float dh_alpha;
	float dh_d;
	float dh_a;
	float dh_min;
	float dh_max;
}

structure JointState {
	JointStateType type;
	float position;
	float goal;
	float speed;
}

structure JointCommand {
	float speed;
	float goal;
}

structure PlanSegment {
	JointCommand joints[];
}

structure TrajectorySegment {
	Point location;
	Rotation rotation;
	bool required;
	float gripper;
	float speed;
}

message ManipulatorDescription {
	string name;
	float version;
	JointDescription joints[];
}

message ManipulatorState {
	ManipulatorStateType state;
	JointState joints[];
}

message Plan {
	PlanSegment segments[];
}

message Trajectory {
	TrajectorySegment segments[];
}
