#!/usr/bin/env python
import sys
import os
import hashlib
import echolib
import traceback
import signal

from manus.messages import PrivilegedCommand, PrivilegedCommandType, PrivilegedCommandSubscriber

from .system import SystemHandler
from .network import NetworkHandler
from .hardware import HardwareHandler

if __name__ == '__main__':

    if len(sys.argv) > 1:
        h = hashlib.new('sha1')
        h.update(sys.argv[1])
        print(h.hexdigest())
        sys.exit(0)

    testing = bool(os.getenv('MANUS_TESTING', False))
    secret = os.getenv('MANUS_SECRET', None)
    loop = echolib.IOLoop()

    client = echolib.Client(name="privileged")
    loop.add_handler(client)

    handlers = {
        PrivilegedCommandType.SYSTEM: SystemHandler(client),
        PrivilegedCommandType.NETWORK: NetworkHandler(client),
        PrivilegedCommandType.HARDWARE: HardwareHandler(client),
    }

    def control_callback(command):
        h = hashlib.new('sha1')
        h.update(command.secret)
        if secret and h.hexdigest() != secret:
            print("Unauthorized command")
            return
        print("Running privileged command: %s" % PrivilegedCommandType.str(command.command))
        if testing:
            return
        try:
            if command.command in handlers:
                handlers[command.command].handle(command.arguments)
        except Exception as e:
            print(traceback.format_exc())

    def shutdown_handler(signum, frame):
        for handler in handlers.values():
            handler.stop()
        sys.exit(0)

    #signal.signal(signal.SIGINT, shutdown_handler)
    signal.signal(signal.SIGTERM, shutdown_handler)

    control = PrivilegedCommandSubscriber(client, "privileged", control_callback)

    try:
        while loop.wait(1000):
            for handler in handlers.values():
                handler.run()
    except KeyboardInterrupt:
        pass
    finally:
        pass

    shutdown_handler(0, None)
