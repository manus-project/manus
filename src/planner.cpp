#include <iostream>
#include <memory>
#include <functional>
#include <cmath>

#include <kdl/joint.hpp>
#include <kdl/frames.hpp>

#include "voxelgrid.h"

#include <manus/manipulator.h>

using namespace echolib;
using namespace manus::manipulator;
using namespace std;

#define RADIAN_TO_DEGREE(X) ((X * 180) / M_PI )
#define DEGREE_TO_RADIAN(X) ((X / 180) * M_PI )

class Planner {
public:
	Planner(SharedClient client) {
		description_subscriber = make_shared<TypedSubscriber<ManipulatorDescription> >(client,
		                         "description", bind(&Planner::on_description, this, std::placeholders::_1));
		state_subscriber = make_shared<TypedSubscriber<ManipulatorState> >(client, "state",
		                   bind(&Planner::on_state, this, std::placeholders::_1));
		trajectory_subscriber = make_shared<TypedSubscriber<Trajectory> >(client, "trajectory",
		                        bind(&Planner::on_trajectory, this, std::placeholders::_1));
		plan_publisher = make_shared<TypedPublisher<Plan> >(client, "plan");

	}

	virtual ~Planner() {

	}

	void idle() {
		if (cache) {
			cout << "Precomputing ..." << endl;
			cache->precompute(100);
		}
	}

protected:

	void on_description(shared_ptr<ManipulatorDescription> desc) {

		kinematic_chain = Chain();

		vector<float> lmin;
		vector<float> lmax;
		vector<float> spos;

		for (size_t j = 0; j < desc->joints.size(); j++) {

			JointDescription joint = desc->joints[j];

			switch (joint.type) {
			case ROTATION: {
				kinematic_chain.addSegment(Segment(Joint(Joint::RotZ), Frame::DH(joint.dh_a, DEGREE_TO_RADIAN(joint.dh_alpha), joint.dh_d, 0)));
				lmin.push_back(DEGREE_TO_RADIAN(joint.dh_min));
				lmax.push_back(DEGREE_TO_RADIAN(joint.dh_max));
				spos.push_back(DEGREE_TO_RADIAN(joint.dh_theta));
				break;
			}
			case TRANSLATION: {
				kinematic_chain.addSegment(Segment(Joint(Joint::TransZ), Frame::DH(joint.dh_a, DEGREE_TO_RADIAN(joint.dh_alpha), 0, DEGREE_TO_RADIAN(joint.dh_theta))));
				lmin.push_back(joint.dh_min);
				lmax.push_back(joint.dh_max);
				spos.push_back(joint.dh_d);
				break;
			}
			case GRIPPER: {
				gripper = j;
			}
			case FIXED: {
				kinematic_chain.addSegment(Segment(Joint(Joint::None), Frame::DH(joint.dh_a, DEGREE_TO_RADIAN(joint.dh_alpha), joint.dh_d, DEGREE_TO_RADIAN(joint.dh_theta))));
				break;
			}
			}
		}

		limits_min = JntArray(lmin.size());
		limits_max = JntArray(lmax.size());
		safe = JntArray(lmax.size());

		for (size_t j = 0; j < lmin.size(); j++) {
			limits_min(j) = lmin[j];
			limits_max(j) = lmax[j];
			safe(j) = spos[j];
		}


		cache = make_shared<VoxelGrid>(kinematic_chain, limits_min, limits_max, 0.1, 40, 100);

		description_subscriber.reset();

		cache->precompute(5000);
	}


	void on_state(shared_ptr<ManipulatorState> s) {
		state = s;
	}

	void on_trajectory(shared_ptr<Trajectory> trajectory) {

		if (!cache || !state || trajectory->segments.empty())
			return;

		Plan plan;

		JntArray initial = state_to_array(*state);

		for (size_t i = 0; i < trajectory->segments.size(); i++) {

			TrajectorySegment goal = trajectory->segments[i];
			PlanSegment segment;

			KDL::Rotation rotation = KDL::Rotation::Identity().RotX(goal.rotation.x).RotY(goal.rotation.y).RotZ(goal.rotation.z);
			//KDL::Rotation rotation(goal.rotation.x, goal.rotation.y, goal.rotation.z);
			KDL::Frame frame(rotation, KDL::Vector(goal.location.x, goal.location.y, goal.location.z));

cout << goal.location.x << ", " <<  goal.location.y << ", " << goal.location.z << endl;

			JntArray out(kinematic_chain.getNrOfJoints());
			int result = cache->CartToJnt(initial, frame, out);

			if (result >= 0) {
				initial = out;
				segment = array_to_plan(out, goal.speed);
				segment.joints[gripper].goal = goal.gripper;
				plan.segments.push_back(segment);
			} else {
				cout << "Plan unsuccessful" << endl;
				return;
			}

		}



		plan_publisher->send(plan);

	}

	JntArray state_to_array(const ManipulatorState& state) {

		JntArray res(kinematic_chain.getNrOfJoints());

		int j = 0;
		for (int i = 0; i < kinematic_chain.getNrOfSegments(); i++) {
			if (kinematic_chain.getSegment(i).getJoint().getType() == Joint::None)
				continue;
			res(j++) = state.joints[i].position;
		}

		return res;

	}

	PlanSegment array_to_plan(const JntArray& array, float speed) {

		PlanSegment res;

		int j = 0;
		for (int i = 0; i < kinematic_chain.getNrOfSegments(); i++) {
			JointCommand joint;
			if (kinematic_chain.getSegment(i).getJoint().getType() == Joint::None) {
				joint.goal = 0;
				joint.speed = speed;
			} else {
				joint.goal = kinematic_chain.getSegment(i).getJoint().getType() == Joint::RotZ ? RADIAN_TO_DEGREE(array(j++)) : array(j++);
				joint.speed = speed;
			}
			res.joints.push_back(joint);
		}

		return res;


	}

private:

	SharedTypedSubscriber<ManipulatorDescription> description_subscriber;
	SharedTypedSubscriber<ManipulatorState> state_subscriber;
	SharedTypedSubscriber<Trajectory> trajectory_subscriber;
	SharedTypedPublisher<Plan> plan_publisher;

	Chain kinematic_chain;
	shared_ptr<VoxelGrid> cache;

	shared_ptr<ManipulatorState> state;

	JntArray limits_max, limits_min, safe;

	int gripper;
};

int main(int argc, char** argv) {

	IOLoop loop;

	SharedClient client = connect(loop);

	Planner planner(client);

	while (loop.wait(5000)) {
		planner.idle();


	}

	exit(0);
}

