import sys
import os
import json
import hashlib
import shlex
import signal
import traceback

import echolib

from manus.apps import AppCommandType, AppEventType, AppListingPublisher, AppEventPublisher, AppCommandSubscriber, AppEvent, AppData, AppListing

__author__ = 'lukacu'

def terminate_process(proc_pid):
    process = psutil.Process(proc_pid)
    for proc in process.get_children(recursive=True):
        proc.terminate()
    process.terminate()

class Application(object):

    def __init__(self, appfile, listed=True):
        absfile = os.path.abspath(appfile)
        try:
            with open(appfile) as f:
                content = f.readlines()
            digest = hashlib.md5()
            digest.update(appfile)
            self.identifier = digest.hexdigest()
            self.name = content[0].strip()
            self.version = int(content[1].strip())
            self.script = content[2].strip()
            self.path = os.path.dirname(absfile)
            if len(content) > 3:
                self.description = "".join(content[3:])
            else:
                self.description = ""
            self.dir = os.path.dirname(appfile)
            self.listed = listed
        except ValueError, e:
            raise Exception("Unable to read app file %s" % appfile)

        self.process = None

    def __str__(self):
        return self.name

    def run(self):
        if self.process:
            return

        import subprocess

        command = "python %s" % os.path.join(self.path, self.script)
        environment = os.environ.copy()
        environment.update(environment)
        environment["APPLICATION_ID"] = self.identifier
        environment["APPLICATION_NAME"] = self.name
        environment["APPLICATION_PATH"] = self.path
        command = os.path.expandvars(command)
        self.process = subprocess.Popen(shlex.split(command), stdin=subprocess.PIPE,
             stdout=sys.stdout, stderr=subprocess.STDOUT, env=environment,
             shell=False, bufsize=1, cwd=self.dir)

    def alive(self):
        if not self.process:
            return False
        return self.process.poll() is None

    def stop(self):
        if not self.process:
            return
        self.process.kill()
        self.process = None

    def kill(self):
        if not self.process:
            return
        self.process.kill()
        self.process = None

    def message_data(self):
        d = AppData()
        d.name = self.name
        d.id = self.identifier
        d.script = self.script
        d.version = self.version
        d.description = self.description
        d.listed = self.listed
        return d

def scan_applications(pathlist):
    applications = {}
    for path in pathlist:
        for dirpath, dirnames, filenames in os.walk(path):
            for filename in filenames[:]:
                if not filename.endswith(".app"):
                    continue
                try:
                    app = Application(os.path.join(dirpath, filename))
                    applications[app.identifier] = app
                except Exception, e:
                    print traceback.format_exc()
                    continue
    return applications

def find_by_name(applications, name):
    for identifier, app in applications.items():
        if app.name == name:
            return identifier
    return None

def application_launcher(autorun=None):

    app_path = os.environ.get("APPS_PATH", "")

    print "Scanning for applications"
    applications = scan_applications(app_path.split(os.pathsep))
    print "Loaded %d applications" % len(applications)

    global active_application
    active_application = None

    def start_application(identifier):
        global active_application
        starting_application = None

        if identifier and not applications.has_key(identifier):
            if identifier.endswith(".app") and os.path.isfile(identifier):
                try:
                    starting_application = Application(identifier, listed=False)
                except Exception, e:
                    print traceback.format_exc()
                    return
            else:
                print "Application does not exist"
                return
        else:
            starting_application = applications[identifier]

        if active_application:
            terminate = active_application
            active_application = None
            terminate.stop()

        if not identifier:
            event = AppEvent()
            event.type = AppEventType.ACTIVE
            event.app = AppData()
            announce.send(event)
            return

        active_application = starting_application
        active_application.run()
        print "Starting application %s (%s)" % (active_application.name, identifier)
        event = AppEvent()
        event.type = AppEventType.ACTIVE
        event.app = active_application.message_data()
        announce.send(event)

    def control_callback(command):
        try:
            if command.type == AppCommandType.EXECUTE:
                start_application(command.arguments[0])
        except Exception, e:
            print traceback.format_exc()

    def shutdown_handler(signum, frame):
        print "Stopping application"
        if active_application:
            active_application.stop()

    signal.signal(signal.SIGTERM, shutdown_handler)

    loop = echolib.IOLoop()
    client = echolib.Client()
    loop.add_handler(client)

    control = AppCommandSubscriber(client, "apps.control", control_callback)
    announce = AppEventPublisher(client, "apps.announce")
    listing = AppListingPublisher(client, "apps.list")

    if autorun:
        start_application(find_by_name(applications, autorun))

    try:
        while loop.wait(1000):
            # Announce list every second
            message = AppListing()
            for identifier, app in applications.items():
                message.apps.append(app.message_data())
            listing.send(message)
        if not active_application is None:
            if not active_application.alive():
                event = AppEvent()
                event.type = AppEventType.ACTIVE
                event.app = AppData()
                announce.send(event)
                active_application = None
    except KeyboardInterrupt:
        pass
    finally:
        pass

    shutdown_handler(0, None)

