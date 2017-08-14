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

def manus_cotrol_gripper(cmd):
    print "manus_cotrol_gripper('%s')" % cmd
    if (cmd == "open"):
        workspace.manipulator.joint(6, 0)
    elif (cmd == "close"):  
        workspace.manipulator.joint(6, 0.8)

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
        manus.MoveTo((p[0], p[1], p[2]), (0.0, 0.0, angle), 0)
    ])

def manus_detect_blocks():
    print "manus_detect_blocks()"
    return workspace.detect_blocks() # dont't forget to return for manus_detect_and_store_blocks block

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
    print "Blockly code execution Finished!"

except KeyboardInterrupt:
    pass