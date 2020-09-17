#!/usr/bin/env python

import os
import sys
import time
import struct
import traceback

from manus.messages import PrivilegedCommand, PrivilegedCommandType, PrivilegedCommandPublisher
from manus.messages import ConfigBatch, ConfigEntry, ConfigEntryPublisher, ConfigBatchSubscriber

class PrivilegedClient(object):

    def __init__(self, client, secret=""):
        self.pub = PrivilegedCommandPublisher(client, "privileged")
        self._secret = secret

    def request_shutdown(self):
        command = PrivilegedCommand(secret=self._secret)
        command.command = PrivilegedCommandType.SYSTEM
        command.arguments.append("shutdown")
        self.pub.send(command)

    def request_restart(self):
        command = PrivilegedCommand(secret=self._secret)
        command.command = PrivilegedCommandType.SYSTEM
        command.arguments.append("restart")
        self.pub.send(command)

    def request_wifi(self, ssid=None, passphrase="", ap=False):
        command = PrivilegedCommand(secret=self._secret)
        command.command = PrivilegedCommandType.WIFI
        if not ssid is not None:
            command.arguments.append(ssid)
            command.arguments.append(passphrase)
            if ap:
                command.arguments.append("ap")
        self.pub.send(command)

class ConfigManager(object):

    def __init__(self, client):
        self.store = {}
        self._number = 0
        self._listener = ConfigBatchSubscriber(client, "config.notify", self._config_set_callback)
        self._update = ConfigEntryPublisher(client, "config.update")
        self._subscribers = []

    def set(self, key, value):
        if self.store.get(key, "") == value:
            return

        self._update.publish(ConfigEntry(key, value))
        
    def get(self, key):
        return self.store.get(key, "")

    def _config_set_callback(self, batch):
        updated = []
        for entry in batch.entries:
            if self.store.get(entry.key, "") == entry.value:
                continue
            self.store[entry.key] = entry.value
            updated.append(entry.key)

        if updated:
            for subscriber in self._subscribers:
                subscriber(self, updated)

    def listen(self, callback):
        self._subscribers.append(callback)

    def unlisten(self, callback):
        self._subscribers.remove(callback)
