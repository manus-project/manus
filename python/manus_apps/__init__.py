#!/usr/bin/env python
import sys

import echolib
from manus.apps import AppCommandType, AppEventType, AppListingSubscriber, AppEventSubscriber, AppCommandPublisher, AppCommand

class AppsManager(object):

    def __init__(self, client):
        self._listsub = AppListingSubscriber(client, "apps.list", lambda x: self._list(x))
        self._annsub = AppEventSubscriber(client, "apps.announce", lambda x: self._announce(x))
        self._control = AppCommandPublisher(client, "apps.control")
        self._apps = {}
        self._listeners = []

    def listen(self, listener):
        self._listeners.append(listener)

    def unlisten(self, listener):
        self._listeners.remove(listener)

    def list(self):
        return self._apps

    def run(self, id):
        if len(id) > 0:
            msg = AppCommand()
            msg.type = AppCommandType.EXECUTE
            msg.arguments.append(id)
            self._control.send(msg)

    def _list(self, msg):
        self._apps = {v.id: {'identifier': v.id, 'name': v.name, 'version': v.version, 'description' : v.description} for v in msg.apps if v.listed}

    def _announce(self, msg):
        if msg.type == AppEventType.START:
            for s in self._listeners:
                s.on_app_started(msg.app)
        if msg.type == AppEventType.STOP:
            for s in self._listeners:
                s.on_app_stopped(msg.app)
