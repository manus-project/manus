#!/usr/bin/python

import numpy as np
import scipy.spatial.distance
import cv2
import sys
from random import shuffle

from math import *

import manus

from manus_apps.blocks import block_color_name
from manus_apps.blocks import Block
from manus_apps.workspace import Workspace

class BlocklyRuntimeError(RuntimeError):
    def __init__(self, message):
        super(BlocklyRuntimeError, self).__init__(message)

class RobotArm(object):
    def __init__(self):
        self.workspace = Workspace() # Workspace(bounds=[(100, -200), (360, -200), (360, 200), (100, 200)])


    def move_joint(self, joint, angle):
        # Angle is in degrees so we convert it to radians
        angle = float(angle) * pi / 180.0
        # Angle is also between 0 and 360 so we shift by -180

        # TODO: also do some joint-angle-limit checks here?

        # Move joint
        self.workspace.manipulator.joint(joint, angle)


    def cotrol_gripper(self, cmd):
        if (cmd == "open"):
            self.workspace.manipulator.joint(6, 0)
        if (cmd == "half"):
            self.workspace.manipulator.joint(6, 0.5)
        elif (cmd == "close"):  
            self.workspace.manipulator.joint(6, 1)


    def wait(self, milisecs):
        self.workspace.wait(milisecs)


    def move_to_coordinates(self, p):
        if (p[0] is None or p[1] is None or p[2] is None):
            raise BlocklyRuntimeError("Coordinates don't appear to be set. This can happen if no detection block was used.")
        # TODO: Calculate appropriate angle here
        self.workspace.manipulator
        angle = 0.0
        joints = self.workspace.manipulator.state().joints
        gripper = joints[-1].goal
        # Move arm
        self.workspace.manipulator.trajectory([
            manus.MoveTo((p[0], p[1], p[2]), (0.0, 0.0, angle), gripper)
        ])


    def detect_blocks(self):
        blocks = self.workspace.detect_blocks() # dont't forget to return for manus_detect_and_store_blocks block
        shuffle(blocks)
        return blocks


    def get_joint_position(self, joint):
        joints = self.workspace.manipulator.state().joints
        if joint < 1 or joint > len(joints):
            raise BlocklyRuntimeError("Illegal joint number")
        return joints[joint].position


def retrieve_coordinate_from_point(comp, point):
    if isinstance(point, tuple) and len(point) == 3:
        if comp == "x":
            return point[0]
        elif comp == "y":
            return point[1]
        elif comp == "z":
            return point[2]
    if isinstance(point, Block):
        if comp == "x":
            return point.position[0]
        elif comp == "y":
            return point.position[1]
        elif comp == "z":
            return point.position[2]
    raise BlocklyRuntimeError("Not a point or a block object")


def retrieve_color_from_block(block):
    if isinstance(block, Block):
        return block_color_name(block)
    else:
        raise BlocklyRuntimeError("Not a block object")