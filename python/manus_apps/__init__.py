#!/usr/bin/env python
import sys
import os
import hashlib
import echolib
from manus.messages import AppCommandType, AppEventType, AppListingSubscriber, AppEventSubscriber, \
                     AppCommandPublisher, AppCommand, AppStreamDataSubscriber, AppStreamDataPublisher, \
                     AppStreamData

def app_identifier(appfile):
    absfile = os.path.abspath(appfile)
    digest = hashlib.md5()
    digest.update(absfile)
    return digest.hexdigest()

class AppsManager(object):

    def __init__(self, client):
        self._listsub = AppListingSubscriber(client, "apps.list", lambda x: self.on_list(x))
        self._annsub = AppEventSubscriber(client, "apps.announce", lambda x: self.on_announce(x))
        self._control = AppCommandPublisher(client, "apps.control")
        self._output = AppStreamDataSubscriber(client, "apps.output", lambda x: self.on_output(x))
        self._input = AppStreamDataPublisher(client, "apps.input")
        self._apps = {}
        self._listeners = []
        self._active = None

    def listen(self, listener):
        self._listeners.append(listener)

    def unlisten(self, listener):
        self._listeners.remove(listener)

    def list(self):
        return self._apps

    def active(self):
        return self._active

    def run(self, id):
        msg = AppCommand()
        msg.type = AppCommandType.EXECUTE
        msg.arguments.append(id)
        self._control.send(msg)

    def input(self, lines):
        if self._active is None:
            return
        msg = AppStreamData()
        msg.id = self._active.id
        msg.lines = lines

        self._input.send(msg)

        for s in self._listeners:
            s.on_app_input(msg.id, msg.lines)

    def on_list(self, msg):
        self._apps = {v.id: {'identifier': v.id, 'name': v.name, 'version': v.version, 'description' : v.description} for v in msg.apps if v.listed}

    def on_output(self, msg):
        if self._active is None or self._active.id != msg.id:
            return
        for s in self._listeners:
            s.on_app_output(msg.id, msg.lines)

    def on_announce(self, msg):
        if msg.type == AppEventType.ACTIVE:
            if len(msg.app.id) == 0:
                self._active = None
            else:
                self._active = msg.app
            for s in self._listeners:
                s.on_app_active(self._active)
