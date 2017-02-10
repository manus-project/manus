import sys
import os
import json
import hashlib
import shlex
import signal

import echolib

__author__ = 'lukacu'

CONTAINERS = {"webox" : "${WEBOX_DIR}/webox.sh %(path)s",
      "flashbox" : "/usr/bin/wine ${FLASH_PLAYER} %(swf)s",
      "python" : "python %(path)s/application.py"}

def terminate_process(proc_pid):
    process = psutil.Process(proc_pid)
    for proc in process.get_children(recursive=True):
        proc.terminate()
    process.terminate()

class Application(object):

    def __init__(self, identifier, name, path, description='', version=1, container=None, categories=[], metadata={}):
        self.path = path
        self.identifier = identifier
        self.name = name
        self.description = description
        self.version = version
        self.container = container
        self.categories = categories
        self.metadata = metadata
        self.process = None
        if container and not CONTAINERS.has_key(container):
          raise ValueError("Unknown container type %s" % container)

    def __str__(self):
        return self.name

    def run(self):
        if self.process:
            return

        import subprocess

        variables = self.metadata.copy()
        variables.update({"path" : self.path, "name": self.name, "version": self.version})

        if self.container:
            command = CONTAINERS[self.container] % variables
        else:
            command = os.path.join(path, self.name)
        environment = os.environ.copy()
        environment.update(environment)
        environment["APPLICATION_ID"] = self.identifier
        environment["APPLICATION_NAME"] = self.name
        environment["APPLICATION_PATH"] = self.path
        command = os.path.expandvars(command)

        #environment["LD_PRELOAD"] = '/home/lukacu/Local/matrix/libxparam.so'
        self.process = subprocess.Popen(shlex.split(command), stdin=subprocess.PIPE,
             stdout=sys.stdout, stderr=subprocess.STDOUT, env=environment,
             shell=False, bufsize=1, cwd=self.path) # preexec_fn=os.setsid

    def stop(self):
        if not self.process:
            return
        self.process.kill()
        #terminate_process(self.pid)
        #os.killpg(self.process.pid, signal.SIGTERM)
        self.process = None

    def kill(self):
        if not self.process:
            return
        self.process.kill()
        self.process = None

def scan_applications(pathlist):
    applications = {}
    for path in pathlist:
        for dirpath, dirnames, filenames in os.walk(path):
            for dirname in dirnames[:]:
                manifest = os.path.join(dirpath, dirname, 'application.json')
                if not os.path.exists(manifest):
                    continue
                try:
                    metadata = json.load(open(manifest, 'r'))
                    digest = hashlib.md5()
                    digest.update(manifest)
                    identifier = digest.hexdigest()
                except ValueError, e:
                    print e
                    continue
                try:
                    # Legacy support
                    if metadata.has_key("identifier"):
                        del metadata["identifier"]
                    applications[identifier] = Application(identifier,
                        path=os.path.dirname(manifest), **metadata)
                except TypeError, e:
                    print e
                    continue
                except ValueError, e:
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
        if not identifier:
            return

        if not applications.has_key(identifier):
            print "Application does not exist"
            return

        if active_application:
            terminate = active_application
            active_application = None
            terminate.stop()
            message = echolib.Dictionary()
            message["event"] = "stop"
            message["name"] = terminate.name
            message["identifier"] = terminate.identifier
            announce.send(message)

        active_application = applications[identifier]
        #if terminate.identifier == active_application.identifier
        active_application.run()
        print "Running application %s (%s)" % (active_application.name, identifier)
        message = echolib.Dictionary()
        message["event"] = "start"
        message["name"] = active_application.name
        message["identifier"] = active_application.identifier
        announce.send(message)

    def control_callback(message):
        command = message.get("command", "unknown")
        if command == "run":
            identifier = message.get("identifier", "")
            if not identifier:
                name = message.get("name", "")
                identifier = find_by_name(applications, name)
                if not identifier:
                    print "Application does not exist"
                    return
            start_application(identifier)

    def shutdown_handler():
        print "Stopping application"
        if active_application:
            active_application.stop()

    signal.signal(signal.SIGTERM, shutdown_handler)

    def publish_list():
        pass

    loop = echolib.IOLoop()
    client = echolib.Client()
    loop.add_handler(client)

    control = echolib.DictionarySubscriber(client, "app_control", control_callback)
    announce = echolib.DictionaryPublisher(client, "app_announce")
    listing = echolib.DictionaryPublisher(client, "app_list")

    if autorun:
        start_application(find_by_name(applications, autorun))

    try:
        while loop.wait(1000):
            # Announce list every second
            message = echolib.Dictionary()
            for identifier, app in applications.items():
                message[identifier] = app.name
            listing.send(message)
    except KeyboardInterrupt:
        pass
    finally:
        pass

    shutdown_handler()

