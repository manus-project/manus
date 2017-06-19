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

def evaluate_block(workspace, blocks, active, groups):

    block = blocks[active]

    group = block_color_name(block)

    if not group in groups:
        return False

    r = groups[group]["region"]
    p = block.position
    if (p[0] > r[0]) and (p[1] > r[1]) and (p[0] < r[0] + r[2]) and (p[1] < r[1] + r[3]):
        return False

    padding = 20
    targets = np.concatenate((np.random.uniform(r[0] + padding, r[0] + r[2] - padding, (100,1)),  np.random.uniform(r[1] + padding, r[1] + r[3] - padding, (100,1))), axis=1)
    obstacles = np.array([b.position[0:2] for b in blocks], dtype='float32')
    D = scipy.spatial.distance.cdist(targets, obstacles, 'euclidean')
    targets = targets[np.all(D > 50, 1)]
    target = None

    if len(targets) < 1:
        return False

    print "Moving %s box" % group
    t = targets[0]
    angle1 = block.rotation[2]
    angle2 = atan2(t[1], t[0])

    trajectory = [
        manus.MoveTo((p[0], p[1], 120.0), (0.0, 0.0, angle1), 0.0),
        manus.MoveTo((p[0], p[1], 40.0), (0.0, 0.0, angle1), 0.0),
        manus.MoveTo((p[0], p[1], 60.0), (0.0, 0.0, angle1), 0.8),
        manus.MoveTo((p[0], p[1], 120.0), (0.0, 0.0, angle1), 0.8),
        manus.MoveTo((t[0], t[1], 120.0), (0.0, 0.0, angle2), 0.8),
        manus.MoveTo((t[0], t[1], 70.0), (0.0, 0.0, angle2), 0.8),
        manus.MoveTo((t[0], t[1], 70.0), (0.0, 0.0, angle2), 0.0),
        manus.MoveTo((t[0], t[1], 120.0), (0.0, 0.0, angle2), 0.0)
    ]

    workspace.manipulator.trajectory(trajectory)

    return True

workspace = Workspace(bounds=[(100, -200), (360, -200), (360, 200), (100, 200)])

groups = {
    "red": {"region" : [120, -100, 120, 80], "color": [255, 255, 0]},
    "blue": {"region" : [120, 20, 120, 80], "color": [0, 255, 255]}
}

try:

    while True:
        workspace.wait(1000)

        blocks = workspace.detect_blocks()
        shuffle(blocks)

        print "Detected %d blocks" % len(blocks)

        for i, block in enumerate(blocks):
            if evaluate_block(workspace, blocks, i, groups):
                break

except KeyboardInterrupt:
    pass

