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

workspace = Workspace() #bounds=[(100, -200), (360, -200), (360, 200), (100, 200)])

detection_color = None
detection_x = None
detection_y = None
detection_z = None

def manus_move_joint(joint, angle):
    # Angle is in degrees so we convert it to radians
    angle = float(angle) * pi / 180.0
    # Angle is also between 0 and 360 so we shift by -180

    # TODO: also do some joint-angle-limit checks here?

    # Move joint
    workspace.manipulator.joint(joint, angle)

def manus_cotrol_gripper(cmd):
    if (cmd == "open"):
        workspace.manipulator.joint(6, 0)
    if (cmd == "half"):
        workspace.manipulator.joint(6, 0.5)
    elif (cmd == "close"):  
        workspace.manipulator.joint(6, 1)

def manus_wait(milisecs):
    workspace.wait(milisecs)

def manus_move_arm_to_coordinates(p):
    if (p[0] is None or p[1] is None or p[2] is None):
        raise BlocklyRuntimeError("Coordinates don't appear to be set. This can happen if no detection block was used.")
    # TODO: Calculate appropriate angle here
    workspace.manipulator
    angle = 0.0
    joints = workspace.manipulator.state().joints
    gripper = joints[-1].goal
    # Move arm
    workspace.manipulator.trajectory([
        manus.MoveTo((p[0], p[1], p[2]), (0.0, 0.0, angle), gripper)
    ])

def manus_any_block_detected():
    # Set detection variables!
    global detection_color, detection_x, detection_y, detection_z 
    detection_color = "red"
    detection_x = 200
    detection_y = 100
    detection_z = 100
    return True # Dont't forget to return something

def manus_detect_blocks():
    blocks = workspace.detect_blocks() # dont't forget to return for manus_detect_and_store_blocks block
    shuffle(blocks)
    return blocks

def manus_get_position():
    pass

def manus_get_joint(joint):
    joints = workspace.manipulator.state().joints
    if joint < 1 or joint > len(joints):
        raise BlocklyRuntimeError("Illegal joint number")
    return joints[joint].position

def manus_retrieve_coordinate_from_point(comp, point):
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

def manus_retrieve_color_from_block(block):
    if isinstance(block, Block):
        return block_color_name(block)
    else:
        raise BlocklyRuntimeError("Not a block object")

try:
    workspace.wait(500)
{{code_container}}

except BlocklyRuntimeError, e:
    print "Terminated because of an error during execution: ", e

except KeyboardInterrupt:
    pass