#!/usr/bin/python

# import the necessary packages
import numpy as np
import cv2
import sys
from manus.messages import JointType, PlanStateType, Marker, Markers, MarkersPublisher, MarkerOperation
import manus
import echolib
import echocv
import random

from manus_apps.blocks import BlockDetector, block_color

class Camera(object):

    def __init__(self, workspace, name):
        self.image = None
        self.location = None
        self.parameters = None
        self.workspace = workspace        
        self.frame_sub = echocv.FrameSubscriber(workspace.client, "%s.image" % name, lambda x: self._frame_callback(x))
        self.parameters_sub = echocv.CameraIntrinsicsSubscriber(
            workspace.client, "%s.parameters" % name, lambda x: self._parameters_callback(x))
        self.location_sub = echocv.CameraExtrinsicsSubscriber(
            workspace.client, "%s.location" % name, lambda x: self._location_callback(x))
        self.name = name

    def _frame_callback(self, frame):
        self.image = frame.image

    def _location_callback(self, location):
        self.location = location

    def _parameters_callback(self, parameters):
        self.parameters = parameters

    def is_ready(self):
        return not (self.location == None or self.parameters == None)

    def get_image(self):
        return self.image

    def get_homography(self):
        if not self.location:
            return np.identity(3)

        transform = np.concatenate((self.location.rotation, np.transpose(self.location.translation)), axis=1)
        projective = np.matmul(self.parameters.intrinsics, transform)
        self.homography = projective[:, [0, 1, 3]]
        self.homography = self.homography / self.homography[2, 2]

        return self.homography

    def get_rotation(self):
        return self.location.rotation

    def get_translation(self):
        return self.location.translation

    def get_intrinsics(self):
        return self.parameters.intrinsics

    def get_distortion(self):
        return self.parameters.distortion

    def get_width(self):
        return self.image.shape[1]

    def get_height(self):
        return self.image.shape[0]

    def __str__(self):
        return "Camera %s" % self.name

class Manipulator(object):

    def __init__(self, workspace, identifier):
        self.handle = manus.Manipulator(workspace.client, identifier)
        self.handle.listen(self)
        self.move_waiting = None
        self.move_result = None
        self.identifier = str(random.getrandbits(128))
        self.workspace = workspace
        self.current_state = None

    def is_ready(self):
        return not self.current_state == None

    def on_manipulator_state(self, manipulator, state):
        self.current_state = state

    def on_planner_state(self, manipulator, state):
        if state.identifier == self.move_waiting:
            if state.type == PlanStateType.COMPLETED:
                self.move_result = True
                self.move_waiting = None
            if state.type == PlanStateType.FAILED:
                self.move_result = False
                self.move_waiting = None

    def state(self):
        return self.current_state
    
    def safe(self):
        self.handle.move_safe(self.identifier)
        return self.wait_for(self.identifier)

    def transform(self, joint):
        return self.handle.transform(joint)

    def position(self, joint):
        t = self.handle.transform(joint)
        return (t[0, 3], t[1, 3], t[2, 3])

    def move(self, location, rotation):
        gripper = self.current_state.joints[-1].goal
        trajectory = [manus.MoveTo(location, rotation, gripper)]
        self.handle.trajectory(self.identifier, trajectory)
        return self.wait_for(self.identifier)

    def trajectory(self, trajectory):
        self.handle.trajectory(self.identifier, trajectory)
        return self.wait_for(self.identifier)

    def joint(self, joint, goal):
        self.handle.move_joint(joint, goal, identifier=self.identifier)
        return self.wait_for(self.identifier)

    def gripper(self, goal=None):
        if goal is None:
            return self.current_state.joints[-1].goal
        return self.joint(len(self.current_state.joints)-1, goal)

    def wait_for(self, identifier):
        self.move_waiting = identifier
        while True:
            self.workspace.loop.wait(10)
            if self.move_waiting is None:
                return self.move_result

    def __str__(self):
        return "Manipulator %s" % self.identifier


class Workspace(object):

    def __init__(self, bounds=None):
        self.loop = echolib.IOLoop()
        self.client = echolib.Client(name=os.environ.get("APPLICATION_NAME", ""))
        self.loop.add_handler(self.client)
        self.bounds = bounds
        self.camera = Camera(self, "camera0")
        self.manipulator = Manipulator(self, "manipulator0")
        self.markers_pub = MarkersPublisher(self.client, "markers")

        while not (self.camera.is_ready() and self.manipulator.is_ready()):
            self.wait(100)

    def detect_blocks(self, block_size=20):
        if not self.manipulator.safe():
            raise Exception("Unable to move to safe position")
        self.wait(500)
        detector = BlockDetector(block_size=block_size, bounds=self.bounds)
    
        blocks = detector.detect(self.camera)

        markers = Markers()
        markers.operation = MarkerOperation.OVERWRITE
        for i, b in enumerate(blocks):
            m = Marker()
            m.id = str(i)
            m.location.x = b.position[0]
            m.location.y = b.position[1]
            m.location.z = b.position[2]
            m.rotation.x = b.rotation[0]
            m.rotation.y = b.rotation[1]
            m.rotation.z = b.rotation[2]
            m.size.x = b.size[0]
            m.size.y = b.size[1]
            m.size.z = b.size[2]
            c = block_color(b)
            m.color.red = c[0]
            m.color.green = c[1]
            m.color.blue = c[2]
            markers.markers.append(m)
        self.markers_pub.send(markers)

        return blocks

    def wait(self, duration=10):
        self.loop.wait(duration)


    def __str__(self):
        return "Workspace"


