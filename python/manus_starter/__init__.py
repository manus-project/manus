#!/usr/bin/env python2

import os
import sys
import time
import struct
import signal
import shutil

from ignition import ProgramGroup

try:
    from manus import VERSION
except ImportError:
    VERSION = 'N/A'

LOCAL_CAMERA = os.getenv('MANUS_CAMERA', '/dev/video0')
LOCAL_MANIPULATOR = os.getenv('MANUS_MANIPULATOR', '/dev/i2c-1')

kill_now = False

def exit_gracefully(signum, frame):
    global kill_now
    kill_now = True
    print "Stopping gracefully"


def get_ip_address(ifname):
    try:
        import socket
        import fcntl
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        return socket.inet_ntoa(fcntl.ioctl(s.fileno(), 0x8915,  # SIOCGIFADDR
            struct.pack('256s', ifname[:15])
        )[20:24])
    except IOError:
        return "unknown"


class Menu(object):

    def __init__(self, items, stdscreen):
        self.window = stdscreen.subwin(0, 0)
        self.window.keypad(1)
        self.panel = panel.new_panel(self.window)
        self.panel.hide()
        panel.update_panels()
        self.position = 0
        self.items = items

    def navigate(self, n):
        self.position += n
        if self.position < 0:
            self.position = 0
        elif self.position >= len(self.items):
            self.position = len(self.items)-1

    def display(self):
        self.panel.top()
        self.panel.show()
        self.window.clear()
        self.window.timeout(1000)

        self.update_footer()

        while not kill_now:
            try:
                self.window.refresh()
                curses.doupdate()
                for index, item in enumerate(self.items):
                    if index == self.position:
                        mode = curses.A_REVERSE
                    else:
                        mode = curses.A_NORMAL

                    msg = '%d. %s' % (index+1, item[0])
                    self.window.addstr(3+index, 3, msg, mode)

                key = self.window.getch()

                if key in [curses.KEY_ENTER, ord('\n')]:
                    if self.items[self.position][1] is None:
                        break
                    else:
                        self.items[self.position][1]()

                elif key == curses.KEY_UP:
                    self.navigate(-1)

                elif key == curses.KEY_DOWN:
                    self.navigate(1)

                elif key == -1:
                    self.update_footer()
            except KeyboardInterrupt:
                pass

        self.window.clear()
        self.panel.hide()
        panel.update_panels()
        curses.doupdate()

    def update_footer(self):
        s = self.window.getmaxyx()
        has_camera, has_manipulator = scan_resources()
        datetime = time.strftime("%Y-%m-%d %H:%M:%S", time.gmtime())
        metadata = "%s Manus %s" % (datetime, VERSION)
        camera = "ON" if has_camera else "OFF"
        manipulator = "ON" if has_manipulator else "OFF"
        local_address = get_ip_address(LOCAL_INTERFACE)
        devices = "%s Camera %s Manipulator %s" % (
            ManusInit.local_address, camera, manipulator)
        self.window.move(s[0]-3, 0)
        self.window.clrtoeol()
        self.window.addstr(s[0]-3, 3, devices, curses.A_REVERSE)
        self.window.addstr(s[0]-3, s[1] - 3 - len(metadata),
                           metadata, curses.A_REVERSE)


def run_script(script):
    print "Running %s" % script
    try:
        if isinstance(script, ProgramGroup):
            group = script
        else:
            group = ProgramGroup(script)

        group.announce("Starting up Manus ...")
        group.start()
        time.sleep(1)

    except ValueError, e:
        print "Error opening spark file %s" % e
        return False

    try:
        while group.valid():
            time.sleep(1)
            if not scan_resources() or kill_now:
                break
    except KeyboardInterrupt:
        return False

    group.stop()
    group.announce("Shutting down Manus ...")

    return True


def scan_resources():
    has_camera = os.path.exists(LOCAL_CAMERA)
    has_manipulator = os.path.exists(LOCAL_MANIPULATOR)
    return has_camera, has_manipulator


def run_interactive(launchfiles):
    import curses
    from curses import panel

    signal.signal(signal.SIGINT, exit_gracefully)
    signal.signal(signal.SIGTERM, exit_gracefully)

    menu_items = []

    for file in os.listdir(launchfiles):
        if not file.endswith(".spark"):
            continue
        try:
            group = ProgramGroup(os.path.join(launchfiles, file))
            menu_items.append((group.title, lambda x: no_curses(run_script, group)))
        finally:
            pass
            
    screen = stdscreen
    curses.curs_set(0)

    def no_curses(fun, **kwargs):
        curses.noecho()
        curses.cbreak()
        self.screen.keypad(1)

        fun(**kwargs)

        self.screen.clear()
        curses.nocbreak()
        self.screen.keypad(0)
        self.screen.move(0, 0)
        curses.echo()
        curses.endwin()

    def run_exit_curses(self):
        curses.noecho()
        curses.cbreak()
        self.screen.keypad(1)
        kill_now = True

    menu_items.append(
        ('Upgrade system', lambda x: no_curses(run_upgrade)))
    menu_items.append(('Exit to terminal', run_exit_curses))
    menu_items.append(('Shutdown', lambda x: no_curses(run_shutdown)))

    self.screen.clear()
    curses.nocbreak()
    self.screen.keypad(0)
    self.screen.move(0, 0)
    curses.echo()
    curses.endwin()

    menu=Menu(menu_items, screen)
    menu.display()

def run_service(launchfile):
    signal.signal(signal.SIGINT, exit_gracefully)
    signal.signal(signal.SIGTERM, exit_gracefully)
    try:
        while not kill_now:
            if scan_resources():
                if not run_script(launchfile):
                    break
            else:
                time.sleep(3)
    except KeyboardInterrupt:
        return