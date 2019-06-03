#!/usr/bin/env python
from __future__ import absolute_import

import sys
import json
import uuid
import os
import signal
import traceback

from manus.messages import JointType, PlanStateType

from .utilities import synchronize, JsonHandler, NumpyEncoder
import tornado.web

from manus import MoveTo, MoveJoints

from jsonschema import validate, ValidationError

TRAJECTORY_SCHEMA = {
     "type" : "array",
     "minItems": 1,
     "items" : {
        "type" : "object",
        "properties" : {
            "location" : {"type" : "array", "items" : [{"type": "number"}, {"type": "number"}, {"type": "number"}]},
            "rotation" : {"type" : "array", "items" : [{"type": "number"}, {"type": "number"}, {"type": "number"}]},
            "grip" : {"type" : "number"},
            "speed" : {"type" : "number"}
        },
        "required" : ["location"]
     }
}

MOVE_SCHEMA = {
     "type" : "array",
     "minItems": 1,
     "items" : {
        "type" : "object",
        "properties" : {
            "goals" : {"type" : "array", "items" : {"type": "number"}},
            "speed" : {"type" : ["number", "array"], "items": {"type": "number"}}
        }
     }
}

JOINT_SCHEMA = {
    "type" : "object",
    "properties" : {
            "id" : {"type" : "integer"},
            "goal" : {"type" : "number"},
            "speed" : {"type" : "number"}
        }
}

class ManipulatorBlockingHandler(JsonHandler):

    def __init__(self, application, request, manipulator):
        super(ManipulatorBlockingHandler, self).__init__(application, request)
        self.manipulator = manipulator
        self.moveid = uuid.uuid4().hex

    def get(self):
        self.clear()
        self.write_error(404, 'Bad request')
        self.finish()
        return

    @tornado.web.asynchronous
    def post(self):
        try:
            blocking = self.request.arguments.get("blocking", "0")[0]
            blocking = (blocking.lower() in ("yes", "true", "1"))
            if blocking:
                self.manipulator.listen(self)

            result = self.run(self.moveid)

            if not blocking:
                if result:
                    self.response = {'result' : 'ok'}
                else:
                    self.response = {'result' : 'error'}
                self.write_json()
                self.finish()
        except ValueError:
            self.write_error(401, message="Illegal data")
            self.finish()

    def on_finish(self):
        self.manipulator.unlisten(self)

    def on_connection_close(self):
        self.manipulator.unlisten(self)

    def on_manipulator_state(self, manipulator, state):
        pass

    def on_planner_state(self, manipulator, state):
        if state.identifier == self.moveid:
            if state.type == PlanStateType.COMPLETED:
                self.response = {'result' : 'ok'}
            elif state.type == PlanStateType.FAILED:
                self.response = {'result' : 'failed'}
            elif state.type == PlanStateType.STOPPED:
            	self.response = {'result' : 'canceled'}
            else:
            	return
            self.write_json()
            self.finish()

    def check_etag_header(self):
        return False

class ManipulatorDescriptionHandler(JsonHandler):

    def __init__(self, application, request, manipulator):
        super(ManipulatorDescriptionHandler, self).__init__(application, request)
        self.manipulator = manipulator

    def get(self):
        if getattr(self.manipulator, 'description', None) is None:
            self.clear()
            self.set_status(400)
            self.finish('Unavailable')
            return
        description = self.manipulator.description
        self.response = ManipulatorDescriptionHandler.encode_description(description)
        self.write_json()

    @staticmethod
    def encode_description(description):
        joints = []
        for j in description.joints:
            joints.append({"type" : JointType.str(j.type), "theta" : j.dh_theta,
                    "alpha" : j.dh_alpha, "d" : j.dh_d, "a" : j.dh_a, "min" : j.dh_min, "max" : j.dh_max} )
        origin = {"x" : description.frame.origin.x, "y" : description.frame.origin.y, "z" : description.frame.origin.z}
        rotation = {"x" : description.frame.rotation.x, "y" : description.frame.rotation.y, "z" : description.frame.rotation.z}
        return {"name": description.name, "version": description.version, "joints" : joints, "offset" : {"origin": origin, "rotation": rotation}}

    def check_etag_header(self):
        return False

class ManipulatorStateHandler(JsonHandler):

    def __init__(self, application, request, manipulator):
        super(ManipulatorStateHandler, self).__init__(application, request)
        self.manipulator = manipulator

    def get(self):
        state = self.manipulator.state
        if state is None:
            self.clear()
            self.set_status(400)
            self.finish('Unavailable')
            return
        self.set_header('X-Timestamp', state.header.timestamp.isoformat())
        self.response = ManipulatorStateHandler.encode_state(state)
        self.write_json()

    @staticmethod
    def encode_state(state):
        return {"joints" : [ {"position" : j.position, "goal": j.goal} for j in state.joints]}

    def check_etag_header(self):
        return False

class ManipulatorMoveJointHandler(ManipulatorBlockingHandler):

    def __init__(self, application, request, manipulator):
        super(ManipulatorMoveJointHandler, self).__init__(application, request, manipulator)

    def run(self, id):
        try:

            validate(instance = self.request.json, schema = JOINT_SCHEMA)
            joint = self.request.json.get("id")
            goal = float(self.request.json.get("goal"))
            speed = float(self.request.json.get("speed", 1.0))
            self.manipulator.move_joint(joint, goal, speed, identifier=id)
            return True

        except ValidationError, e:
            return False

class ManipulatorMoveHandler(ManipulatorBlockingHandler):

    def __init__(self, application, request, manipulator):
        super(ManipulatorMoveHandler, self).__init__(application, request, manipulator)

    def run(self, id):
        try:

            validate(instance = self.request.json, schema = MOVE_SCHEMA)
            states = [MoveJoints(goal["goals"], goal.get("speed", 1.0)) for goal in self.request.json]
            self.manipulator.move(id, states)

            return True

        except ValidationError:
            return False

    def check_etag_header(self):
        return False

class ManipulatorTrajectoryHandler(ManipulatorBlockingHandler):

    def __init__(self, application, request, manipulator):
        super(ManipulatorTrajectoryHandler, self).__init__(application, request, manipulator)

    def run(self, id):
        try:

            validate(instance = self.request.json, schema = TRAJECTORY_SCHEMA)

            goals = [MoveTo(goal["location"], goal.get("rotation", None), goal.get("grip", 0), goal.get("speed", 1.0)) for goal in self.request.json]
            self.manipulator.trajectory(id, goals)

            return True

        except ValidationError:
            return False

    def check_etag_header(self):
        return False

class ManipulatorMoveSafeHandler(ManipulatorBlockingHandler):

    def __init__(self, application, request, manipulator):
        super(ManipulatorMoveSafeHandler, self).__init__(application, request, manipulator)

    def run(self, id):
        self.manipulator.move_safe(identifier=id)
        return True
