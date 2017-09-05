import os
import sys
import time
import struct
import signal
import shutil
import traceback

from subprocess import call

from manus.messages import PrivilegedCommand, PrivilegedCommandType, PrivilegedCommandPublisher

class PrivilegedClient(object):

    def __init__(self, client):
        self.pub = PrivilegedCommandPublisher(client, "privileged")

    def request_shutdown(self, credentials):
        command = PrivilegedCommand()
        command.command = PrivilegedCommandType.SHUTDOWN
        self.pub.send(command)

    def request_restart(self, credentials):
        command = PrivilegedCommand()
        command.command = PrivilegedCommandType.REBOOT
        self.pub.send(command)

def run_shutdown():
    try:
        if os.geteuid() == 0:
            call(["sudo", "/sbin/shutdown", "now"])
            return True
        else:
            return False
    except Exception, e:
        print traceback.format_exc()
        return False


def run_restart():
    try:
        if os.geteuid() == 0:
            call(["/sbin/reboot"])
            return True
        else:
            return False
    except Exception, e:
        print traceback.format_exc()
        return False


def run_upgrade():
    try:
        if os.geteuid() == 0:
            call(["sudo", "apt-get", "update"])
            call(["sudo", "apt-get", "upgrade", "-y"])
            call(["sudo", "apt-get", "autoremove", "-y"])
            return True
        else:
            return False
    finally:
        return False
~                     