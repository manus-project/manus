#!/usr/bin/python

# import the necessary packages
import numpy as np
import cv2
import sys
from manus.manipulator import JointType, PlanStateType
import manus
import echolib
import echocv
import random

from manus_apps.blocks import BlockDetector

class Camera(object):

    def __init__(self, workspace, name):
        self.image_sub = echocv.ImageSubscriber(
            workspace.client, "%s.image" % name, lambda x: self._image_callback(x))
        self.parameters_sub = echocv.CameraIntrinsicsSubscriber(
            workspace.client, "%s.parameters" % name, lambda x: self._parameters_callback(x))
        self.location_sub = echocv.CameraExtrinsicsSubscriber(
            workspace.client, "%s.location" % name, lambda x: self._location_callback(x))
        self.image = None
        self.location = None
        self.parameters = None
        self.workspace = workspace

    def _image_callback(self, image):
        self.image = image

    def _location_callback(self, location):
        self.location = location

    def _parameters_callback(self, parameters):
        self.parameters = parameters

    def get_image(self):
        return self.image

    def get_homography(self):
        if not self.location:
            return np.identity(3)

        transform = np.concatenate((self.location.rotation, self.location.translation), axis=1)
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

class Manipulator(object):

    def __init__(self, workspace, identifier):
        self.handle = manus.Manipulator(workspace.client, identifier)
        self.handle.listen(self)
        self.move_waiting = None
        self.move_result = None
        self.identifier = str(random.getrandbits(128))
        self.workspace = workspace

    def on_manipulator_state(self, manipulator, state):
        pass

    def on_planner_state(self, manipulator, state):
        if state.identifier == self.move_waiting:
            if state.type == PlanStateType.COMPLETED:
                self.move_result = True
                self.move_waiting = None
            if state.type == PlanStateType.FAILED:
                self.move_result = False
                self.move_waiting = None
    
    def safe(self):
        self.handle.move_safe(self.identifier)
        return self.wait_for(self.identifier)

    def move(self, location, rotation):
        trajectory = [manus.MoveTo(location, rotation, 0.0)]
        self.handle.trajectory(self.identifier, trajectory)
        return self.wait_for(self.identifier)

    def trajectory(self, trajectory):
        self.handle.trajectory(self.identifier, trajectory)
        return self.wait_for(self.identifier)

    def joint(self, joint, goal):
        self.handle.move_joint(self, joint, goal, identifier=self.identifier)
        return self.wait_for(self.identifier)

    def wait_for(self, identifier):
        self.move_waiting = identifier
        while True:
            self.workspace.loop.wait(10)
            if not self.move_waiting:
                return self.move_result


class Workspace(object):

    def __init__(self, bounds=None):
        self.loop = echolib.IOLoop()
        self.client = echolib.Client()
        self.loop.add_handler(self.client)
        self.bounds = bounds
        self.camera = Camera(self, "camera0")
        self.manipulator = Manipulator(self, "manipulator0")

    def detect_blocks(self, block_size=20):
        if not self.manipulator.safe():
            raise Exception("Unable to move to safe position")
        self.wait(500)
        detector = BlockDetector(block_size=block_size, bounds=self.bounds)
    
        blocks = detector.detect(self.camera)
        return blocks

    def wait(self, duration=10):
        self.loop.wait(duration)





