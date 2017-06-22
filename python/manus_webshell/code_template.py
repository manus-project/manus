#!/usr/bin/python

import numpy as np
import scipy.spatial.distance
import cv2
import sys
from random import shuffle

from math import *

import manus

from manus_apps.blocks import block_color_name
from manus_apps.workspace import Workspace
#from manus_webshell.markers import Markers

workspace = Workspace(bounds=[(100, -200), (360, -200), (360, 200), (100, 200)])

def manus_move_joint(joint, angle):
    print "manus_move_joint("+str(joint)+", "+str(angle)+")"
    # Angle is in degrees so we convert it to radians
    angle = float(angle) * pi / 180.0
    # Angle is also between 0 and 360 so we shift by -180
    angle -= pi

    # TODO: also do some joint-angle-limit checks here?

    # Move joint
    workspace.manipulator.move_joint(id, position, speed)


def manus_wait(milisecs):
    print "manus_wait("+str(milisecs)+")"
    workspace.wait(milisecs)

def manus_move_arm_to_coordinates(p):
    print "manus_move_arm_to_coordinates("+str(p)+")"
    # TODO: Calculate appropriate angle here
    angle = 0.0
    # Move arm
    workspace.manipulator.trajectory([
        manus.MoveTo((p[0], p[1], p[2]), (0.0, 0.0, angle), 0.0)
    ])

def manus_any_block_detector:
    print "manus_any_block_detector"
    return True # Dont't forget to return something

def manus_colored_block_detector(color):
    print "manus_colored_block_detector('"+color+"')"
    return True # Dont't forget to return something

try:
    print "Starting Blockly code execution"
{{code_container}}

except KeyboardInterrupt:
    pass
except Exception as e:
    print "Blockly code execution failed: "+e.message