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

    def list(self):
        return self._apps

    def run(self, id):
        if len(id) > 0:
            msg = AppCommand
            msg.type = "RUN"
            msg.id = id
            self._control.send(msg)

    def _list(self, msg):
        self._apps = {v.id: {'identifier': v.id, 'name': v.name, 'version': v.version} for v in msg.apps}

    def _announce(self, msg):
        pass
