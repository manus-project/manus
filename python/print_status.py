
import curses
from curses import panel


import echolib
import random
import math
from manus.manipulator import ManipulatorDescriptionSubscriber, ManipulatorStateSubscriber, TrajectorySegment, JointCommand, Trajectory, TrajectorySubscriber, Point, Rotation

description = None

class Wrapper(object):

    update_resources = 0

    def __init__(self, stdscreen):
        self.screen = stdscreen
        curses.curs_set(0)
        self.screen.clear()
        loop = echolib.IOLoop()
        client = echolib.Client(loop, "/tmp/echo.sock")

        state = ManipulatorStateSubscriber(
            client, "manipulator0.state", self.on_state)
        commands = TrajectorySubscriber(
            client, "manipulator0.trajectory", self.on_trajectory)

        while loop.wait(30):
            pass

    def on_trajectory(self, obj):
        for i, j in zip(range(len(obj.joints)), obj.joints):
            msg = '%d. %.2f' % (i+1, j.goal)
            self.screen.addstr(30+i, 3, msg, curses.A_REVERSE)

    def on_state(self, obj):
        self.screen.refresh()
        curses.doupdate()
        for i, j in zip(range(len(obj.joints)), obj.joints):
            msg = '%d. %.2f %.2f' % (i+1, j.goal, j.position)
            self.screen.addstr(3+i, 3, msg, curses.A_REVERSE)



if __name__ == '__main__':
    curses.wrapper(Wrapper)
