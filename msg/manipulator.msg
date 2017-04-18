namespace manus.manipulator;

import "geometry.msg";
import "headers.msg";

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
	JointCommand[] joints;
}

structure TrajectorySegment {
	Point3D location;
	Rotation3D rotation;
	bool required;
	float gripper;
	float speed;
}

message ManipulatorDescription {
	string name;
	float version;
	JointDescription[] joints;
}

message ManipulatorState {
	ManipulatorStateType state;
	JointState[] joints;
}

message PlanState {
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

