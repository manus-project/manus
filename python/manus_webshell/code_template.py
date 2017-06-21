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

def manus_move_arm_to_coordinates(coordinates):
    print "manus_move_arm_to_coordinates("+coordinates+")"

try:
{{code_container}}

except KeyboardInterrupt:
    pass
except Exception as e:
    print "Blockly code execution failed: "+e.message