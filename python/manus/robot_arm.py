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

class RobotArm(object):
    def __init__(self):
        self.workspace = Workspace(bounds=[(100, -200), (360, -200), (360, 200), (100, 200)])


    def move_joint(self, joint, angle):
        print "manus_momove_jointve_joint("+str(joint)+", "+str(angle)+")"
        # Angle is in degrees so we convert it to radians
        angle = float(angle) * pi / 180.0
        # Angle is also between 0 and 360 so we shift by -180
        angle -= pi

        # TODO: also do some joint-angle-limit checks here?

        # Move joint
        self.workspace.manipulator.joint(joint, angle)

    def cotrol_gripper(self, cmd):
        print "cotrol_gripper('%s')" % cmd
        if (cmd == "open"):
            self.workspace.manipulator.joint(6, 0)
        elif (cmd == "close"):  
            self.workspace.manipulator.joint(6, 0.8)

    def wait(self, milisecs):
        print "wait("+str(milisecs)+")"
        self.workspace.wait(milisecs)

    def move_to_coordinates(self, p):
        print "move_to_coordinates("+str(p)+")"
        if (p[0] is None or p[1] is None or p[2] is None):
            raise RuntimeError("Coordinates don't appear to be set. This can happen if no detection block was used.")
        # TODO: Calculate appropriate angle here
        angle = 0.0
        # Move arm
        self.workspace.manipulator.trajectory([
            manus.MoveTo((p[0], p[1], p[2]), (0.0, 0.0, angle), 0)
        ])

    def detect_blocks(self):
        print "detect_blocks()"
        return self.workspace.detect_blocks() # dont't forget to return for manus_detect_and_store_blocks block

def manus_retrieve_component_from_block(comp, block):
    print "manus_retrieve_component_from_block('"+comp+"', block)"
    if comp == "x":
        return block.position[0]
    elif comp == "y":
        return block.position[1]
    elif comp == "z":
        return block.position[2]
    elif comp == "color":
        return block_color_name(block)