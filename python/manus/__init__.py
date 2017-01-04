
import manus.manipulator as messages

NAME = 'Manus'
VERSION = 'N/A'

try:
    with open('/usr/share/manus/version', 'r') as f:
        VERSION = f.read()
except IOError:
    pass

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

    def listen(self, listener):
        self._listeners.append(listener)

    def unlisten(self, listener):
        self._listeners.remove(listener)

    def description(self):
        return self._description.data

    def move(self, joints, speed = 1.0):
        plan = messages.Plan()
        segment = messages.PlanSegment()
        for j in range(len(self.state.joints)):
            c = messages.JointCommand()
            c.speed = speed
            c.goal = joints[j]
            segment.joints.append(c)
        plan.segments.append(segment)
        self._move.send(plan)

    def move_joint(self, joint, goal, speed = 1.0):
        segment = self._state_to_segment()
        segment.joints[joint].speed = speed
        segment.joints[joint].goal = goal
        plan = messages.Plan()
        plan.segments.append(segment)
        self._move.send(plan)
        self.state.joints[joint].goal = goal

    def _state_callback(self, state):
        self.state = state
        for s in self._listeners:
            s.push_manipulator_state(self, state)

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
