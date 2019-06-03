import "geometry.msg";

enumerate ManipulatorStateType { UNKNOWN, CONNECTED, PASSIVE, ACTIVE, CALIBRATION }
enumerate JointType { ROTATION, TRANSLATION, GRIPPER, FIXED }
enumerate JointStateType { IDLE, MOVING, ERROR }

enumerate PlanStateType { PLANNING, FAILED, PLANNED, RUNNING, STOPPED, COMPLETED }
 
structure JointDescription {
	JointType type;
	float dh_theta;
	float dh_alpha;
	float dh_d;
	float dh_a;
	float dh_min;
	float dh_max;
	float dh_safe;
}

structure JointState {
	JointStateType type;
	float position;
	float goal;
	float speed;
}

structure JointCommand {
	float goal;
	float speed;
}

structure PlanSegment {
	JointCommand[] joints;
}

structure TrajectorySegment {
	Frame3D frame;
	bool required = true;
	float gripper = 0;
	float speed = 1;
	bool rotation = true;
}

message ManipulatorDescription {
	string name;
	float version;
	Frame3D frame;
	JointDescription[] joints;
}

message ManipulatorState {
	Header header;
	ManipulatorStateType state;
	JointState[] joints;
}

message PlanState {
	Timestamp timestamp;
	string identifier;
	PlanStateType type;
	long timestamp;
}

message Plan {
	string identifier;
	PlanSegment[] segments;
}

message Trajectory {
	string identifier;
	TrajectorySegment[] segments;
}

