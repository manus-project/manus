
import manus.messages as messages

import numpy as np

NAME = 'Manus'
VERSION = 'N/A'

try:
    with open('/usr/share/manus/version', 'r') as f:
        VERSION = f.read()
except IOError:
    pass


class MoveTo(object):
    def __init__(self, location, rotation, gripper=0):
        self.location = location
        self.rotation = rotation
        self.gripper = gripper

    def generate(self, manipulator):
        seg = messages.TrajectorySegment()
        seg.location = messages.Point3D()
        seg.rotation = messages.Rotation3D()
        seg.location.x = self.location[0]
        seg.location.y = self.location[1]
        seg.location.z = self.location[2]
        seg.rotation.x = self.rotation[0]
        seg.rotation.y = self.rotation[1]
        seg.rotation.z = self.rotation[2]
        seg.gripper = self.gripper
        seg.required = True
        seg.speed = 1.0
        return seg

class Manipulator(object):

    def __init__(self, client, name):
        self.name = name
        self.description = None
        self.state = None
        self._client = client
        self._listeners = []
        self._description = messages.ManipulatorDescriptionSubscriber(client, "%s.description" % name, lambda x: self._description_callback(x))
        self._state = messages.ManipulatorStateSubscriber(client, "%s.state" % name, lambda x: self._state_callback(x))
        self._move = messages.PlanPublisher(client, "%s.plan" % name)
        self._planner = messages.TrajectoryPublisher(client, "%s.trajectory" % name)
        self._planstate = messages.PlanStateSubscriber(client, "%s.planstate" % name, lambda x: self._planstate_callback(x))

    def listen(self, listener):
        self._listeners.append(listener)

    def unlisten(self, listener):
        try:
            self._listeners.remove(listener)
        except ValueError:
            pass

    def description(self):
        return self._description.data

    def move_safe(self, identifier='safe'):
        plan = messages.Plan()
        plan.identifier = identifier
        segment = messages.PlanSegment()
        plan.segments.append(segment)
        self._move.send(plan)
        #self.move({0 : -0.43, 1: 2.22, 2: -1.84, 3: -0.63, 4: 0.0, 5: -1.5, 6: 0.01}, identifier=identifier)

    def move(self, joints, speed = 1.0, identifier='move'):
        plan = messages.Plan()
        plan.identifier = identifier
        segment = messages.PlanSegment()
        for j in range(len(self.state.joints)):
            c = messages.JointCommand()
            c.speed = speed
            c.goal = joints[j]
            segment.joints.append(c)
        plan.segments.append(segment)
        self._move.send(plan)

    def move_joint(self, joint, goal, speed = 1.0, identifier='move joint'):
        segment = self._state_to_segment()
        segment.joints[joint].speed = speed
        segment.joints[joint].goal = goal
        plan = messages.Plan()
        plan.identifier = identifier
        plan.segments.append(segment)
        self._move.send(plan)
        self.state.joints[joint].goal = goal

    def trajectory(self, identifier, goals):
        msg = messages.Trajectory()
        msg.identifier = identifier
        msg.segments = [s.generate(self) for s in goals]
        self._planner.send(msg)

    def _state_callback(self, state):
        self.state = state
        for s in self._listeners:
            s.on_manipulator_state(self, state)

    def _description_callback(self, description):
        self.description = description

    def _state_to_segment(self):
        segment = messages.PlanSegment()
        for j in self.state.joints:
            c = messages.JointCommand()
            c.speed = j.speed
            c.goal = j.goal
            segment.joints.append(c)
        return segment

    def _planstate_callback(self, state):
        for s in self._listeners:
            s.on_planner_state(self, state)


    def transform(self, joint):
        origin = np.identity(4)
        if self.description is None or joint < 0 or joint >= len(self.state.joints):
            return origin
        for j in xrange(0, joint+1):
            dh_a, dh_alpha, dh_d, dh_theta = self.description.joints[j].dh_a, \
                self.description.joints[j].dh_alpha, \
                self.description.joints[j].dh_d, \
                self.description.joints[j].dh_theta

            if self.description.joints[j].type == messages.JointType.ROTATION:
                dh_theta = self.state.joints[j].position
            elif self.description.joints[j].type == messages.JointType.TRANSLATION:
                dh_d = self.state.joints[j].position

            ct = np.cos(dh_theta)
            st = np.sin(dh_theta)
            ca = np.cos(dh_alpha)
            sa = np.sin(dh_alpha)

            transform = np.array(((ct, -st * ca, st * sa, dh_a * ct), \
                             (st, ct * ca, -ct * sa, dh_a * st), (0, sa, ca, dh_d), (0, 0, 0, 1)))

            origin = np.matmul(origin, transform)

        return origin
