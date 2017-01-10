// This is an autogenerated file, do not modify!

#ifndef __MANIPULATOR_MSGS_H
#define __MANIPULATOR_MSGS_H

#include <echolib/message.h>
#include <echolib/datatypes.h>
#include <vector>



namespace manus {
namespace manipulator {



enum ManipulatorStateType { PASSIVE, UNKNOWN, CONNECTED, ACTIVE, CALIBRATION };

enum JointType { ROTATION, FIXED, TRANSLATION, GRIPPER };

enum JointStateType { IDLE, MOVING, ERROR };


class Point;
class Rotation;
class JointDescription;
class JointState;
class JointCommand;
class PlanSegment;
class TrajectorySegment;
class ManipulatorDescription;
class ManipulatorState;
class Plan;
class Trajectory;



class Point {
public:
	Point() {};
	virtual ~Point() {};
	float x;
	float y;
	float z;
	
};

class Rotation {
public:
	Rotation() {};
	virtual ~Rotation() {};
	float x;
	float y;
	float z;
	
};

class JointDescription {
public:
	JointDescription() {};
	virtual ~JointDescription() {};
	JointType type;
	float dh_theta;
	float dh_alpha;
	float dh_d;
	float dh_a;
	float dh_min;
	float dh_max;
	
};

class JointState {
public:
	JointState() {};
	virtual ~JointState() {};
	JointStateType type;
	float position;
	float goal;
	float speed;
	
};

class JointCommand {
public:
	JointCommand() {};
	virtual ~JointCommand() {};
	float speed;
	float goal;
	
};

class PlanSegment {
public:
	PlanSegment() {};
	virtual ~PlanSegment() {};
	std::vector<JointCommand> joints;
	
};

class TrajectorySegment {
public:
	TrajectorySegment() {};
	virtual ~TrajectorySegment() {};
	Point location;
	Rotation rotation;
	bool required;
	float gripper;
	float speed;
	
};

class ManipulatorDescription {
public:
	ManipulatorDescription() {};
	virtual ~ManipulatorDescription() {};
	string name;
	float version;
	std::vector<JointDescription> joints;
	
};

class ManipulatorState {
public:
	ManipulatorState() {};
	virtual ~ManipulatorState() {};
	ManipulatorStateType state;
	std::vector<JointState> joints;
	
};

class Plan {
public:
	Plan() {};
	virtual ~Plan() {};
	std::vector<PlanSegment> segments;
	
};

class Trajectory {
public:
	Trajectory() {};
	virtual ~Trajectory() {};
	std::vector<TrajectorySegment> segments;
	
};


}
}



namespace echolib {



template <> inline void read(MessageReader& reader, manus::manipulator::ManipulatorStateType& dst) {
	switch (reader.read<int>()) {
	case 2: dst = manus::manipulator::PASSIVE; break;
	case 0: dst = manus::manipulator::UNKNOWN; break;
	case 1: dst = manus::manipulator::CONNECTED; break;
	case 3: dst = manus::manipulator::ACTIVE; break;
	case 4: dst = manus::manipulator::CALIBRATION; break;
	
	}
}

template <> inline void write(MessageWriter& writer, const manus::manipulator::ManipulatorStateType& src) {
	switch (src) {
	case manus::manipulator::PASSIVE: writer.write<int>(2); return;
	case manus::manipulator::UNKNOWN: writer.write<int>(0); return;
	case manus::manipulator::CONNECTED: writer.write<int>(1); return;
	case manus::manipulator::ACTIVE: writer.write<int>(3); return;
	case manus::manipulator::CALIBRATION: writer.write<int>(4); return;
	
	}
}



template <> inline void read(MessageReader& reader, manus::manipulator::JointType& dst) {
	switch (reader.read<int>()) {
	case 0: dst = manus::manipulator::ROTATION; break;
	case 3: dst = manus::manipulator::FIXED; break;
	case 1: dst = manus::manipulator::TRANSLATION; break;
	case 2: dst = manus::manipulator::GRIPPER; break;
	
	}
}

template <> inline void write(MessageWriter& writer, const manus::manipulator::JointType& src) {
	switch (src) {
	case manus::manipulator::ROTATION: writer.write<int>(0); return;
	case manus::manipulator::FIXED: writer.write<int>(3); return;
	case manus::manipulator::TRANSLATION: writer.write<int>(1); return;
	case manus::manipulator::GRIPPER: writer.write<int>(2); return;
	
	}
}



template <> inline void read(MessageReader& reader, manus::manipulator::JointStateType& dst) {
	switch (reader.read<int>()) {
	case 0: dst = manus::manipulator::IDLE; break;
	case 1: dst = manus::manipulator::MOVING; break;
	case 2: dst = manus::manipulator::ERROR; break;
	
	}
}

template <> inline void write(MessageWriter& writer, const manus::manipulator::JointStateType& src) {
	switch (src) {
	case manus::manipulator::IDLE: writer.write<int>(0); return;
	case manus::manipulator::MOVING: writer.write<int>(1); return;
	case manus::manipulator::ERROR: writer.write<int>(2); return;
	
	}
}





template <> inline void read(MessageReader& reader, manus::manipulator::Point& dst) {
	read(reader, dst.x);read(reader, dst.y);read(reader, dst.z);
}

template <> inline void write(MessageWriter& writer, const manus::manipulator::Point& src) {
	write(writer, src.x);write(writer, src.y);write(writer, src.z);
}

template <> inline void read(MessageReader& reader, manus::manipulator::Rotation& dst) {
	read(reader, dst.x);read(reader, dst.y);read(reader, dst.z);
}

template <> inline void write(MessageWriter& writer, const manus::manipulator::Rotation& src) {
	write(writer, src.x);write(writer, src.y);write(writer, src.z);
}

template <> inline void read(MessageReader& reader, manus::manipulator::JointDescription& dst) {
	read(reader, dst.type);read(reader, dst.dh_theta);read(reader, dst.dh_alpha);read(reader, dst.dh_d);read(reader, dst.dh_a);read(reader, dst.dh_min);read(reader, dst.dh_max);
}

template <> inline void write(MessageWriter& writer, const manus::manipulator::JointDescription& src) {
	write(writer, src.type);write(writer, src.dh_theta);write(writer, src.dh_alpha);write(writer, src.dh_d);write(writer, src.dh_a);write(writer, src.dh_min);write(writer, src.dh_max);
}

template <> inline void read(MessageReader& reader, manus::manipulator::JointState& dst) {
	read(reader, dst.type);read(reader, dst.position);read(reader, dst.goal);read(reader, dst.speed);
}

template <> inline void write(MessageWriter& writer, const manus::manipulator::JointState& src) {
	write(writer, src.type);write(writer, src.position);write(writer, src.goal);write(writer, src.speed);
}

template <> inline void read(MessageReader& reader, manus::manipulator::JointCommand& dst) {
	read(reader, dst.speed);read(reader, dst.goal);
}

template <> inline void write(MessageWriter& writer, const manus::manipulator::JointCommand& src) {
	write(writer, src.speed);write(writer, src.goal);
}

template <> inline void read(MessageReader& reader, manus::manipulator::PlanSegment& dst) {
	read(reader, dst.joints);
}

template <> inline void write(MessageWriter& writer, const manus::manipulator::PlanSegment& src) {
	write(writer, src.joints);
}

template <> inline void read(MessageReader& reader, manus::manipulator::TrajectorySegment& dst) {
	read(reader, dst.location);read(reader, dst.rotation);read(reader, dst.required);read(reader, dst.gripper);read(reader, dst.speed);
}

template <> inline void write(MessageWriter& writer, const manus::manipulator::TrajectorySegment& src) {
	write(writer, src.location);write(writer, src.rotation);write(writer, src.required);write(writer, src.gripper);write(writer, src.speed);
}

template <> inline void read(MessageReader& reader, manus::manipulator::ManipulatorDescription& dst) {
	read(reader, dst.name);read(reader, dst.version);read(reader, dst.joints);
}

template <> inline void write(MessageWriter& writer, const manus::manipulator::ManipulatorDescription& src) {
	write(writer, src.name);write(writer, src.version);write(writer, src.joints);
}

template <> inline void read(MessageReader& reader, manus::manipulator::ManipulatorState& dst) {
	read(reader, dst.state);read(reader, dst.joints);
}

template <> inline void write(MessageWriter& writer, const manus::manipulator::ManipulatorState& src) {
	write(writer, src.state);write(writer, src.joints);
}

template <> inline void read(MessageReader& reader, manus::manipulator::Plan& dst) {
	read(reader, dst.segments);
}

template <> inline void write(MessageWriter& writer, const manus::manipulator::Plan& src) {
	write(writer, src.segments);
}

template <> inline void read(MessageReader& reader, manus::manipulator::Trajectory& dst) {
	read(reader, dst.segments);
}

template <> inline void write(MessageWriter& writer, const manus::manipulator::Trajectory& src) {
	write(writer, src.segments);
}





template <> inline string get_type_identifier<manus::manipulator::ManipulatorDescription>() { return string("dd1989630d5c260004842bd32b81e012"); }

template<> inline shared_ptr<Message> echolib::Message::pack<manus::manipulator::ManipulatorDescription >(const manus::manipulator::ManipulatorDescription &data) {
    MessageWriter writer;
    write(writer, data);
    return make_shared<BufferedMessage>(writer);
}

template<> inline shared_ptr<manus::manipulator::ManipulatorDescription > echolib::Message::unpack<manus::manipulator::ManipulatorDescription>(SharedMessage message) {
    MessageReader reader(message);
    shared_ptr<manus::manipulator::ManipulatorDescription> result(new manus::manipulator::ManipulatorDescription());
    read(reader, *result);
    return result;
}




template <> inline string get_type_identifier<manus::manipulator::ManipulatorState>() { return string("cc43704573b22f6a05e12b5615aa8704"); }

template<> inline shared_ptr<Message> echolib::Message::pack<manus::manipulator::ManipulatorState >(const manus::manipulator::ManipulatorState &data) {
    MessageWriter writer;
    write(writer, data);
    return make_shared<BufferedMessage>(writer);
}

template<> inline shared_ptr<manus::manipulator::ManipulatorState > echolib::Message::unpack<manus::manipulator::ManipulatorState>(SharedMessage message) {
    MessageReader reader(message);
    shared_ptr<manus::manipulator::ManipulatorState> result(new manus::manipulator::ManipulatorState());
    read(reader, *result);
    return result;
}




template <> inline string get_type_identifier<manus::manipulator::Plan>() { return string("cf6fb0339b954d09c854f9a3cb2f1004"); }

template<> inline shared_ptr<Message> echolib::Message::pack<manus::manipulator::Plan >(const manus::manipulator::Plan &data) {
    MessageWriter writer;
    write(writer, data);
    return make_shared<BufferedMessage>(writer);
}

template<> inline shared_ptr<manus::manipulator::Plan > echolib::Message::unpack<manus::manipulator::Plan>(SharedMessage message) {
    MessageReader reader(message);
    shared_ptr<manus::manipulator::Plan> result(new manus::manipulator::Plan());
    read(reader, *result);
    return result;
}




template <> inline string get_type_identifier<manus::manipulator::Trajectory>() { return string("dc40378b85222525ac9a3aef0953dde3"); }

template<> inline shared_ptr<Message> echolib::Message::pack<manus::manipulator::Trajectory >(const manus::manipulator::Trajectory &data) {
    MessageWriter writer;
    write(writer, data);
    return make_shared<BufferedMessage>(writer);
}

template<> inline shared_ptr<manus::manipulator::Trajectory > echolib::Message::unpack<manus::manipulator::Trajectory>(SharedMessage message) {
    MessageReader reader(message);
    shared_ptr<manus::manipulator::Trajectory> result(new manus::manipulator::Trajectory());
    read(reader, *result);
    return result;
}



}

#endif