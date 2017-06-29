#!/usr/bin/env python
from __future__ import absolute_import

import sys
import inspect
import cStringIO
import json
import time
import datetime
import uuid
import logging
import logging.handlers
import os.path
import os
import signal
from bsddb3 import db 

import tornado.httpserver
import tornado.ioloop
import tornado.web
import tornado.wsgi
import tornado.websocket
from tornado.options import options, define, parse_command_line
from tornado.iostream import StreamClosedError

import echolib
import echocv
import echocv.tornado

from .code_generator import CodeGenerator
from .utilities import synchronize, RedirectHandler, DevelopmentStaticFileHandler, JsonHandler, NumpyEncoder
import manus_webshell.static

from manus.messages import JointType, MarkersSubscriber
import manus
from manus_apps import AppsManager, app_identifier
#from manus_privileged import PrivilegedClient

__author__ = 'lukacu'

class ApplicationHandler(JsonHandler):

    def __init__(self, application, request):
        super(ApplicationHandler, self).__init__(application, request)

    def get(self):
        self.response = {
            "name" : manus.NAME,
            "version" : manus.VERSION
        }
        self.write_json()

class CameraDescriptionHandler(JsonHandler):

    def __init__(self, application, request, camera):
        super(CameraDescriptionHandler, self).__init__(application, request)
        self.camera = camera

    def get(self):
        if getattr(self.camera, 'parameters', None) is None:
            self.clear()
            self.set_status(400)
            self.finish('Unavailable')
            return
        parameters = self.camera.parameters
        self.response = {
            "image" : {"width" : parameters.width, "height" : parameters.height},
            "intrinsics" : parameters.intrinsics,
            "distortion" : parameters.distortion
        }
        self.write_json()

class CameraLocationHandler(JsonHandler):

    def __init__(self, application, request, camera):
        super(CameraLocationHandler, self).__init__(application, request)
        self.camera = camera

    @tornado.web.asynchronous
    def get(self):
        self.camera.listen_location(self)

    @staticmethod
    def encode_location(location):
        return {
            "rotation" : location.rotation,
            "translation" : location.translation,
        }

    def push_camera_location(self, camera, location):
        self.response = CameraLocationHandler.encode_location(location)
        self.write_json()
        self.finish()

    def on_finish(self):
        self.camera.unlisten_location(self)

    def on_connection_close(self):
        self.camera.unlisten_location(self)

    def check_etag_header(self):
        return False

class ManipulatorDescriptionHandler(JsonHandler):

    def __init__(self, application, request, manipulator):
        super(ManipulatorDescriptionHandler, self).__init__(application, request)
        self.manipulator = manipulator

    def get(self):
        if getattr(self.manipulator, 'description', None) is None:
            self.clear()
            self.set_status(400)
            self.finish('Unavailable')
            return
        description = self.manipulator.description
        self.response = ManipulatorDescriptionHandler.encode_description(description)
        self.write_json()

    @staticmethod
    def encode_description(description):
        joints = []
        for j in description.joints:
            joints.append({"type" : JointType.str(j.type), "theta" : j.dh_theta,
                    "alpha" : j.dh_alpha, "d" : j.dh_d, "a" : j.dh_a, "min" : j.dh_min, "max" : j.dh_max} )
        return {"name": description.name, "version": description.version, "joints" : joints}

    def check_etag_header(self):
        return False

class ManipulatorStateHandler(JsonHandler):

    def __init__(self, application, request, manipulator):
        super(ManipulatorStateHandler, self).__init__(application, request)
        self.manipulator = manipulator

    def get(self):
        state = self.manipulator.state
        if state is None:
            self.clear()
            self.set_status(400)
            self.finish('Unavailable')
            return
        self.response = ManipulatorStateHandler.encode_state(state)
        self.write_json()

    @staticmethod
    def encode_state(state):
        return {"joints" : [ {"position" : j.position, "goal": j.goal} for j in state.joints]}

    def check_etag_header(self):
        return False

class ManipulatorMoveJointHandler(JsonHandler):

    def __init__(self, application, request, manipulator):
        super(ManipulatorMoveJointHandler, self).__init__(application, request)
        self.manipulator = manipulator

    def get(self):
        if not "id" in self.request.arguments:
            self.clear()
            self.set_status(400)
            self.finish('Unavailable')
            return
        try:
            id = int(self.request.arguments.get("id")[0])
            position = float(self.request.arguments.get("position", "0")[0])
            speed = float(self.request.arguments.get("speed", "1.0")[0])
            self.manipulator.move_joint(id, position, speed)
            self.response = {'result' : 'ok'}
            self.write_json()
        except ValueError:
            self.clear()
            self.set_status(401)
            self.finish('Illegal request')
            return

    def check_etag_header(self):
        return False

class ManipulatorMoveHandler(JsonHandler):

    def __init__(self, application, request, manipulator):
        super(ManipulatorMoveHandler, self).__init__(application, request)
        self.manipulator = manipulator

    def get(self):
        try:
            positions = [float(self.request.arguments.get("j%d" % i, "0")[0]) for i in xrange(1, len(self.manipulator.state.joints)+1)]
            speed = float(self.request.arguments.get("speed", "1.0")[0])
            self.manipulator.move(positions, speed)
            self.response = {'result' : 'ok'}
            self.write_json()
        except ValueError:
            self.clear()
            self.set_status(401)
            self.finish('Illegal request')
            return

    def check_etag_header(self):
        return False

class AppsHandler(JsonHandler):
    def __init__(self, application, request, apps):
        super(AppsHandler, self).__init__(application, request)
        self._apps = apps

    def get(self):
        run = self.request.arguments.get("run", None)
        if not run is None:
            self._apps.run(run[0])
            self.response = {"result" : "ok"}
            self.write_json()
        active = self._apps.active()
        self.response = {"list" : self._apps.list()}
        if not active is None:
            self.response["active"] = active.id
        self.write_json()

    def check_etag_header(self):
        return False

class PrivilegedHandler(JsonHandler):
    def __init__(self, application, request):
        super(PrivilegedHandler, self).__init__(application, request)

    def get(self):
        run = self.request.arguments.get("run", None)
        if not run is None:
            self._apps.run(run[0])
            self.response = {"result" : "ok"}
            self.write_json()
        active = self._apps.active()
        self.response = {"list" : self._apps.list()}
        if not active is None:
            self.response["active"] = active.id
        self.write_json()

    def check_etag_header(self):
        return False

class StorageHandler(tornado.web.RequestHandler):
    keys = []

    def __init__(self, application, request, storage):
        super(StorageHandler, self).__init__(application, request)
        self._storage = storage
        StorageHandler.keys = set(storage.keys())

    def get(self):
        key = self.request.arguments.get("key", [""])[0].strip()
        if not key:
            self.set_header('Content-Type', 'application/json')
            self.finish(json.dumps(list(StorageHandler.keys)))
            return
        try:
            raw = self._storage.get(key, "")
        except db.DBNotFoundError:
            self.set_status(404)
            self.finish("Unknown key")
            return;
        try:
            ctype, data = raw.split(";", 1)
        except ValueError:
            ctype = 'text/plain'
            data = ''
        self.set_header('Content-Type', ctype)
        self.finish(data)

    def post(self):
        key = self.request.arguments.get("key", [""])[0].strip()
        if not key:
            self.set_status(401)
            self.finish('Illegal request')
            return
        try:
            ctype, _ = self.request.headers.get('Content-Type').split(";", 1)
        except ValueError:
            ctype = self.request.headers.get('Content-Type')
        if len(self.request.body) == 0:
            try:
                print "Deleting %s" % key
                self._storage.delete(key)
                StorageHandler.keys.remove(key)
                ApiWebSocket.distribute_message({"channel": "storage", "action" : "delete", "key" : key})
            except db.DBNotFoundError:
                self.finish('Unknown key')
                return
        else:
            data = "%s;%s" % (ctype, self.request.body)
            self._storage.put(key, data)
            StorageHandler.keys.add(key)
            print "Updating %s, content length %d bytes" % (key, len(self.request.body))
            ApiWebSocket.distribute_message({"channel": "storage", "action" : "update", "key" : key, "content" : ctype})
        self.finish()
        
    def check_etag_header(self):
        return False

class ApiWebSocket(tornado.websocket.WebSocketHandler):
    connections = []

    def __init__(self, application, request, cameras, manipulators, apps):
        super(ApiWebSocket, self).__init__(application, request)
        self.cameras = cameras
        self.manipulators = manipulators
        self.apps = apps

    def open(self):
        ApiWebSocket.connections.append(self)
        for c in self.cameras:
            c.listen_location(self)
        for m in self.manipulators:
            m.listen(self)
        self.apps.listen(self)

    def on_message(self, raw):
        message = json.loads(message)
        # can request info about the device?

    def on_close(self):
        ApiWebSocket.connections.remove(self)
        for c in self.cameras:
            c.unlisten_location(self)
        for m in self.manipulators:
            m.unlisten(self)
        self.apps.unlisten(self)

    def send_message(self, message):
        message = json.dumps(message, cls=NumpyEncoder)
        self.write_message(message)

    @staticmethod
    def distribute_message(message):
        message = json.dumps(message, cls=NumpyEncoder)
        for c in ApiWebSocket.connections:
            c.write_message(message)

    def push_camera_location(self, camera, location):
        self.send_message({"channel": "camera", "action" : "update", "data" : CameraLocationHandler.encode_location(location)})

    def on_manipulator_state(self, manipulator, state):
        self.send_message({"channel": "manipulator", "action" : "update", "data" : ManipulatorStateHandler.encode_state(state)})

    def on_planner_state(self, manipulator, state):
        pass

    def on_app_active(self, app):
        if app is None:
            self.send_message({"channel": "apps", "action" : "deactivated" })
        else:
            self.send_message({"channel": "apps", "action" : "activated", "identifier" : app.id})

    def on_app_log(self, identifier, lines):
        self.send_message({"channel": "apps", "action" : "log", "identifier": identifier, "lines" : lines})


class ProgramHandler(JsonHandler):

    def __init__(self, application, request, apps):
        super(ProgramHandler, self).__init__(application, request)
        self._apps = apps

    def get(self):
        self.response = {
            "status" : "error",
            "note" : "You should use POST request"
        }
        self.write_json()

    def post(self):
        self.response = {
            "status" : "ok",
        }
        try:
            current_dir = os.path.dirname(os.path.realpath(__file__))
            generator = CodeGenerator(os.path.join(current_dir, "code_template.tpl"), "/tmp")
            generator.generate_app_with_code(self.request.arguments[u"code"], True)
            self._apps.run("/tmp/generated_app.app")
            self.response["identifier"] = app_identifier("/tmp/generated_app.app")
        except Exception as e:
            self.response = {
                "status" : "error",
                "description" : e.message
            }
       
        self.write_json()

def on_shutdown():
    tornado.ioloop.IOLoop.instance().stop()
    print "Stopping gracefully"

def main():

    logging_level = logging.DEBUG

    logger = logging.getLogger("manus")
    logger.propagate = False

    log_storage = logging.StreamHandler()
    log_storage.setFormatter(logging.Formatter(fmt='%(asctime)s - %(levelname)s\t%(message)s', datefmt='%Y-%m-%d %H:%M:%S'))
    logger.addHandler(log_storage)
    logger.setLevel(logging_level)

    scriptdir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))

    client = echolib.Client()
    storage = db.DB()

    try:

        storagefile = os.getenv('MANUS_STORAGE', "/tmp/manus_storage.db")
        if os.path.exists(storagefile):
            storage.open(storagefile, None, db.DB_HASH)
        else:
            storage.open(storagefile, None, db.DB_HASH, db.DB_CREATE)

        handlers = []
        cameras = []
        manipulators = []

        try:
            camera = echocv.tornado.Camera(client, "camera0")
            handlers.append((r'/api/camera/video', echocv.tornado.VideoHandler, {"camera": camera}))
            handlers.append((r'/api/camera/image', echocv.tornado.ImageHandler, {"camera": camera}))
            handlers.append((r'/api/camera/describe', CameraDescriptionHandler, {"camera": camera}))
            handlers.append((r'/api/camera/position', CameraLocationHandler, {"camera": camera}))
            cameras.append(camera)
        except Exception, e:
            print traceback.format_exc()

        try:
            manipulator = manus.Manipulator(client, "manipulator0")
            handlers.append((r'/api/manipulator/describe', ManipulatorDescriptionHandler, {"manipulator": manipulator}))
            handlers.append((r'/api/manipulator/state', ManipulatorStateHandler, {"manipulator": manipulator}))
            handlers.append((r'/api/manipulator/move_joint', ManipulatorMoveJointHandler, {"manipulator": manipulator}))
            handlers.append((r'/api/manipulator/move', ManipulatorMoveHandler, {"manipulator": manipulator}))
            manipulators.append(manipulator)
        except Exception, e:
            print traceback.format_exc()

        #handlers.append((r'/api/markers', MarkersStorageHandler))
        apps = AppsManager(client)
        handlers.append((r'/api/run', ProgramHandler, {"apps" : apps}))
        handlers.append((r'/api/apps', AppsHandler, {"apps" : apps}))
        handlers.append((r'/api/storage', StorageHandler, {"storage" : storage}))
        handlers.append((r'/api/websocket', ApiWebSocket, {"cameras" : cameras, "manipulators": manipulators, "apps": apps}))
        handlers.append((r'/api/info', ApplicationHandler))
        handlers.append((r'/', RedirectHandler, {'url' : '/index.html'}))
        handlers.append((r'/(.*)', DevelopmentStaticFileHandler, {'path': os.path.dirname(manus_webshell.static.__file__)}))

        def markers_callback(markers):
            data = {m.id : {"location": [m.location.x, m.location.y, m.location.z], "rotation": [m.rotation.x, m.rotation.y, m.rotation.z]} for m in markers.markers}
            print data
            ApiWebSocket.distribute_message({"channel": "markers", "action" : "overwrite", "markers" : data, "overlay" : markers.owner})

        markers_subsriber = MarkersSubscriber(client, "markers", markers_callback)

        application = tornado.web.Application(handlers)

        server = tornado.httpserver.HTTPServer(application)
        server.listen(int(os.getenv('MANUS_PORT', "8080")))

        tornado_loop = tornado.ioloop.IOLoop.instance()

        signal.signal(signal.SIGINT, lambda sig, frame: tornado_loop.add_callback_from_signal(on_shutdown))
        signal.signal(signal.SIGTERM, lambda sig, frame: tornado_loop.add_callback_from_signal(on_shutdown))

        def on_disconnect(client):
            tornado_loop.stop()

        echocv.tornado.install_client(tornado_loop, client, on_disconnect)

        logger.info("Starting %s webshell" % manus.NAME)

        def flush_database():
            # Flush every 5 seconds
            storage.sync()
            tornado_loop.add_timeout(time.time() + 5, flush_database)

        flush_database()

        try:
            tornado_loop.start()
        except KeyboardInterrupt:
            pass
        except Exception, err:
            print traceback.format_exc()

        echocv.tornado.uninstall_client(tornado_loop, client)

    except Exception, e:
        print e

    logger.info("Stopping %s webshell" % manus.NAME)
    storage.close()

if __name__ == '__main__':
    main()


