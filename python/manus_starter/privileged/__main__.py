#!/usr/bin/env python
import sys
import os
import hashlib, binascii
import echolib
from manus.messages import PrivilegedCommand, PrivilegedCommandType, PrivilegedCommandSubscriber

from manus_starter.privileged import *

if __name__ == '__main__':

    if len(sys.argv) > 1:
        h = hashlib.new('sha1')
        h.update(sys.argv[1])
        print h.hexdigest()
        sys.exit(0)

    testing = bool(os.getenv('MANUS_TESTING', False))
    secret = os.getenv('MANUS_SECRET', None)
    loop = echolib.IOLoop()

    def control_callback(command):
        h = hashlib.new('sha1')
        h.update(command.secret)
        if secret and h.hexdigest() != secret:
            print "Unauthorized command"
            return
        print "Running privileged command: %s" % PrivilegedCommandType.str(command.command)
        if testing:
            return
        try:
            if command.type == PrivilegedCommandType.SHUTDOWN:
                manus_privileged.run_shutdown()
            elif command.type == PrivilegedCommandType.REBOOT:
                manus_privileged.run_restart()
            elif command.type == PrivilegedCommandType.UPGRADE:
                manus_privileged.run_upgrade()
        except Exception, e:
            print traceback.format_exc()

    def shutdown_handler(signum, frame):
        sys.exit(0)

    signal.signal(signal.SIGTERM, shutdown_handler)

    client = echolib.Client()
    loop.add_handler(client)

    control = PrivilegedCommandSubscriber(client, "privileged", control_callback)

    try:
        while True:
            loop.wait(100)
    except KeyboardInterrupt:
        pass
    finally:
        pass

    shutdown_handler(0, None)