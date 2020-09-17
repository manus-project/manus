import sys
import os
import json
import shlex
import signal
import traceback
import fcntl
import echolib

from manus.messages import AppCommandType, AppEventType, AppListingPublisher, AppEventPublisher, \
                       AppCommandSubscriber, AppEvent, AppData, AppListing, \
                       AppStreamDataPublisher, AppStreamDataSubscriber, AppStreamData, \
                       AppCommand

from manus_apps import app_identifier

__author__ = 'lukacu'

def terminate_process(proc_pid):
    process = psutil.Process(proc_pid)
    for proc in process.get_children(recursive=True):
        proc.terminate()
    process.terminate()

class Context(object):
    def __init__(self):
        self.active_application = None
        self.applications = {}
        self.loop = echolib.IOLoop()

        self.client = echolib.Client(name="apps")
        self.loop.add_handler(self.client)

        self.control = AppCommandSubscriber(self.client, "apps.control", lambda x: self.control_callback(x))
        self.announce = AppEventPublisher(self.client, "apps.announce")
        self.listing = AppListingPublisher(self.client, "apps.list")
        self.output = AppStreamDataPublisher(self.client, "apps.output")
        self.input = AppStreamDataSubscriber(self.client, "apps.input", lambda x: self.input_callback(x))

    def control_callback(self, command):
        try:
            if command.type == AppCommandType.EXECUTE:
                appid = command.arguments[0] if len(command.arguments) > 0 and len(command.arguments[0]) > 0 else None
                self.start_application(appid)
        except Exception as e:
            print(traceback.format_exc())

    def input_callback(self, message):
        if not self.active_application is None and self.active_application.identifier == message.id:
            for line in message.lines:
                self.active_application.input(line)

    def start_application(self, identifier = None):
        starting_application = None

        if not identifier is None and not self.applications.has_key(identifier):
            starting_application = self.find_by_name(identifier)
            if not starting_application is None:
                pass
            if identifier.endswith(".app") and os.path.isfile(identifier):
                try:
                    starting_application = Application(self, identifier, listed=False)
                except Exception as e:
                    print(traceback.format_exc())
                    return
            else:
                print("Application does not exist")
                return
        elif not identifier is None:
            starting_application = self.applications[identifier]

        if self.active_application:
            terminate = self.active_application
            self.active_application = None
            terminate.stop()

        if starting_application is None:
            event = AppEvent()
            event.type = AppEventType.ACTIVE
            event.app = AppData()
            self.announce.send(event)
            return

        self.active_application = starting_application
        self.active_application.run()
        print("Starting application %s (%s)" % (self.active_application.name, identifier))
        event = AppEvent()
        event.type = AppEventType.ACTIVE
        event.app = self.active_application.message_data()
        self.announce.send(event)

    def scan_applications(self, pathlist):
        for path in pathlist:
            for dirpath, dirnames, filenames in os.walk(path):
                for filename in filenames[:]:
                    if not filename.endswith(".app"):
                        continue
                    try:
                        app = Application(self, os.path.join(dirpath, filename))
                        self.applications[app.identifier] = app
                    except Exception as e:
                        print(traceback.format_exc())
                        continue
        print("Loaded %d applications" % len(self.applications))

    def find_by_name(self, name):
        for identifier, app in self.applications.items():
            if app.name == name:
                return identifier
        return None

    def run(self):
        broadcast = 1
        while self.loop.wait(100):
            broadcast = broadcast - 1
            if broadcast == 0:
                # Announce list every second
                message = AppListing()
                for identifier, app in self.applications.items():
                    message.apps.append(app.message_data())
                self.listing.send(message)
                broadcast = 10
            if not self.active_application is None:
                if not self.active_application.alive():
                    print("Application stopped")
                    event = AppEvent()
                    event.type = AppEventType.ACTIVE
                    event.app = AppData()
                    self.announce.send(event)
                    self.active_application = None

class Application(echolib.IOBase):

    def __init__(self, context, appfile, listed=True):
        super(Application, self).__init__()
        self._fd = -1
        try:
            with open(os.path.abspath(appfile)) as f:
                content = f.readlines()
            self.identifier = app_identifier(appfile)
            self.name = content[0].strip()
            self.version = int(content[1].strip())
            self.script = content[2].strip()
            self.path = os.path.dirname(os.path.abspath(appfile))
            if len(content) > 3:
                self.description = "".join(content[3:])
            else:
                self.description = ""
            self.dir = os.path.dirname(appfile)
            self.listed = listed
            self.context = context
        except ValueError as e:
            raise Exception("Unable to read app file %s" % appfile)

        self.process = None

    def __del__(self):
        print("Deleting app")
        #super(Application, self).__del__()
        self.context.loop.remove_handler(self)

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
             stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=environment,
             shell=False, bufsize=1, cwd=self.dir)
        self._fd = self.process.stdout.fileno()
        fl = fcntl.fcntl(self._fd, fcntl.F_GETFL)
        fcntl.fcntl(self._fd, fcntl.F_SETFL, fl | os.O_NONBLOCK)
        self.context.loop.add_handler(self)

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

    def handle_input(self):
        lines = []
        while True:
            try:
                line = self.process.stdout.readline()
                sys.stdout.write(line)
                lines.append(line)
            except Exception as e: 
                break
        if len(lines) > 0:
            message = AppStreamData()
            message.id = self.identifier
            message.lines = lines
            self.context.output.send(message)
        return self.alive()

    def handle_output(self):
        return True

    def input(self, line):
        self.process.stdin.write(line)

    def fd(self):
        return self._fd

    def disconnect(self):
        self.kill()
        #self.context.loop.remove_handler(self)


def application_launcher():

    app_path = os.environ.get("APPS_PATH", "")
    autorun = os.environ.get("APPS_RUN", None)

    context = Context()

    context.scan_applications(app_path.split(os.pathsep))

    def shutdown_handler(signum, frame):
        context.start_application(None)
        sys.exit(0)

    signal.signal(signal.SIGTERM, shutdown_handler)

    if not autorun is None:
        context.start_application(context.find_by_name(autorun))

    try:
        context.run()
    except KeyboardInterrupt:
        pass
    finally:
        pass

    shutdown_handler(0, None)

def start_app(app):

    from manus.apps import AppCommandType, AppCommandPublisher, AppCommand
    loop = echolib.IOLoop()
    client = echolib.Client(name="appstart")
    loop.add_handler(client)

    control = AppCommandPublisher(client, "apps.control")
    loop.wait(100)
    message = AppCommand()
    message.type = AppCommandType.EXECUTE
    message.arguments.append(app)
    control.send(message)
    loop.wait(100)