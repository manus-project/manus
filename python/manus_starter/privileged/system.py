
import os

from subprocess import call

from manus_starter.privileged import Handler

class SystemHandler(Handler):

    def __init__(self, client):
        pass

    def handle(self, arguments):
 
        try:

            if arguments[0] == "shutdown":
                if os.geteuid() == 0:
                    call(["sudo", "/sbin/shutdown", "now"])
                    return True
                else:
                    return False

            elif arguments[0] == "shutdown":
                if os.geteuid() == 0:
                    call(["/sbin/reboot"])
                    return True
                else:
                    return False

            elif arguments[0] == "upgrade":
                if os.geteuid() == 0:
                    call(["sudo", "apt-get", "update"])
                    call(["sudo", "apt-get", "upgrade", "-y"])
                    call(["sudo", "apt-get", "autoremove", "-y"])
                    return True
                else:
                    return False

        finally:
            return False

