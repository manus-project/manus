import sys
import os
import json
import hashlib
import shlex
import signal

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
            self.name = content[0]
            self.description = content[1]
            self.version = float(content[2])
            self.script = content[3]
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

        variables = self.metadata.copy()
        variables.update({"path" : self.path, "name": self.name, "version": self.version})

        command = "python %s" % self.script
        environment = os.environ.copy()
        environment.update(environment)
        environment["APPLICATION_ID"] = self.identifier
        environment["APPLICATION_NAME"] = self.name
        environment["APPLICATION_PATH"] = self.path
        command = os.path.expandvars(command)
        self.process = subprocess.Popen(shlex.split(command), stdin=subprocess.PIPE,
             stdout=sys.stdout, stderr=subprocess.STDOUT, env=environment,
             shell=False, bufsize=1, cwd=self.dir)

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
        d.version = self.version
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
                    applications[identifier] = Application(os.join(dirpath, script))
                except Exception, e:
                    print e
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
                    print e
                    return
            print "Application does not exist"
            return
        else:
            starting_application = applications[identifier]

        if active_application:
            terminate = active_application
            active_application = None
            terminate.stop()
            event = AppEvent()
            event.event = "STOP"
            event.app = terminate.message_data()
            announce.send(message)

        if not identifier:
            return

        active_application = starting_application
        active_application.run()
        print "Running application %s (%s)" % (active_application.name, identifier)
        event = AppEvent()
        event.event = "START"
        event.app = terminate.message_data()
        announce.send(message)

    def control_callback(command):
        if command.type == AppCommandType.RUN:
            start_application(command.id)

    def shutdown_handler():
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
    except KeyboardInterrupt:
        pass
    finally:
        pass

    shutdown_handler()

