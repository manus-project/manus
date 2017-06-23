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
#from manus_webshell.markers import Markers

workspace = Workspace(bounds=[(100, -200), (360, -200), (360, 200), (100, 200)])

detection_color = None
detection_x = None
detection_y = None
detection_z = None


def manus_move_joint(joint, angle):
    print "manus_move_joint("+str(joint)+", "+str(angle)+")"
    # Angle is in degrees so we convert it to radians
    angle = float(angle) * pi / 180.0
    # Angle is also between 0 and 360 so we shift by -180
    angle -= pi

    # TODO: also do some joint-angle-limit checks here?

    # Move joint
    workspace.manipulator.joint(joint, angle)


def manus_wait(milisecs):
    print "manus_wait("+str(milisecs)+")"
    workspace.wait(milisecs)

def manus_move_arm_to_coordinates(p):
    print "manus_move_arm_to_coordinates("+str(p)+")"
    if (p[0] is None or p[1] is None or p[2] is None):
        raise RuntimeError("Coordinates don't appear to be set. This can happen if no detection block was used.")
    # TODO: Calculate appropriate angle here
    angle = 0.0
    # Move arm
    workspace.manipulator.trajectory([
        manus.MoveTo((p[0], p[1], p[2]), (0.0, 0.0, angle), 0.0)
    ])

def manus_any_block_detected():
    print "manus_any_block_detected"
    # Set detection variables!
    global detection_color, detection_x, detection_y, detection_z 
    detection_color = "red"
    detection_x = 200
    detection_y = 100
    detection_z = 100
    return True # Dont't forget to return something

def manus_block_with_color_detected(color):
    print "manus_block_with_color_detected('"+color+"')"
    # Set detection variables!
    global detection_color, detection_x, detection_y, detection_z
    detection_color = "red"
    detection_x = 200
    detection_y = 100
    detection_z = 100
    return True # Dont't forget to return something

def manus_detect_blocks():

    return [
        Block([100, 100, 100], [0, 0, 0], 20, [0, 0, 0]),
        Block([200, 0, 100], [0, 0, 0], 20, [0, 0, 0]),
        Block([100, -100, 100], [0, 0, 0], 20, [0, 0, 0])
    ] # dont't forget to return for manus_detect_and_store_blocks block

def manus_retrieve_component_from_block(comp, block):
    if comp == "x":
        return block.position[0]
    elif comp == "y":
        return block.position[1]
    elif comp == "z":
        return block.position[2]
    elif comp == "color":
        return block_color_name(block)


try:
    workspace.wait(1000)
    print "Starting Blockly code execution"
{{code_container}}

except KeyboardInterrupt:
    pass