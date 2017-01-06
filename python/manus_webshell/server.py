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

from .utilities import synchronize, RedirectHandler, DevelopmentStaticFileHandler, JsonHandler, NumpyEncoder
import manus_webshell.static

from manus.manipulator import JointType
import manus

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
            joints.append({"type" : j.type, "theta" : j.dh_theta,
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

class ApiWebSocket(tornado.websocket.WebSocketHandler):
    connections = []

    def __init__(self, application, request, cameras, manipulators):
        super(ApiWebSocket, self).__init__(application, request)
        self.cameras = cameras
        self.manipulators = manipulators

    def open(self):
        ApiWebSocket.connections.append(self)
        for c in self.cameras:
            c.listen_location(self)
        for m in self.manipulators:
            m.listen(self)

    def on_message(self, raw):
        message = json.loads(message)
        # can request info about the device?

    def on_close(self):
        ApiWebSocket.connections.remove(self)
        for c in self.cameras:
            c.unlisten_location(self)
        for m in self.manipulators:
            m.unlisten(self)

    @staticmethod
    def distribute_message(message):
        message = json.dumps(message, cls=NumpyEncoder)
        for c in ApiWebSocket.connections:
            c.write_message(message)

    def push_camera_location(self, camera, location):
        self.distribute_message({"type": "update", "object" : "camera", "data" : CameraLocationHandler.encode_location(location)})

    def push_manipulator_state(self, manipulator, state):
        self.distribute_message({"type": "update", "object" : 'manipulator', "data" : ManipulatorStateHandler.encode_state(state)})

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

    try:

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

        handlers.append((r'/api/websocket', ApiWebSocket, {"cameras" : cameras, "manipulators": manipulators}))
        handlers.append((r'/api/app', ApplicationHandler))
        handlers.append((r'/', RedirectHandler, {'url' : '/index.html'}))
        handlers.append((r'/(.*)', DevelopmentStaticFileHandler, {'path': os.path.dirname(manus_webshell.static.__file__)}))

        app = tornado.web.Application(handlers)

        server = tornado.httpserver.HTTPServer(app)
        server.listen(8080)

        tornado_loop = tornado.ioloop.IOLoop.instance()

        def on_disconnect(client):
            tornado_loop.stop()

        echocv.tornado.install_client(tornado_loop, client, on_disconnect)

        logger.info("Starting web console")

        try:
            tornado_loop.start()
        except KeyboardInterrupt:
            logger.info("Stopping web console")
        except Exception, err:
            print traceback.format_exc()

        echocv.tornado.uninstall_client(tornado_loop, client)

    finally:
        pass

if __name__ == '__main__':
    main()


