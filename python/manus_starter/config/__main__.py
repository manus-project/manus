#!/usr/bin/env python
import sys
import os
import hashlib, binascii
import echolib
import traceback
import signal

from manus.messages import ConfigBatch, ConfigEntry, ConfigEntrySubscriber, ConfigBatchPublisher

class ConfigManager(object):

    def __init__(self, client):
        self.store = {}
        self._number = 0
        self._incoming = ConfigEntrySubscriber(client, "update", self._config_set_callback)
        self._notify = ConfigBatchPublisher(client, "notify")
        self._watch = echolib.SubscriptionWatcher(client, "notify", self._subscribers_changed)

    def write_config(self, filename):
        with open(filename, "w") as handle:
            for k, v in self.store.items():
                if k.startswith("_"):
                    continue
                handle.write(k + "=" + v + "\n")

    def read_config(self, filename):
        changes = []
        with open(filename, "r") as handle:
            for line in handle.readlines():
                i = line.find("=")
                if i == -1:
                    continue
                key = line[:i]
                value = line[i+1:]
                if value != self.store.get(key, ""):
                    changes.append(key)
            
        if changes:
            print(changes)
            batch = ConfigBatch()
            for key in changes:
                batch.entries.append(ConfigEntry(key, value))
            self._notify.send(batch)


    def _config_set_callback(self, entry):
        key = entry.key.replace("=", "").replace("\n", "").replace(" ", "")
        value = entry.value.replace("\n", "")

        if value == self.store.get(key, ""):
            return

        self.store[key] = value

        batch = ConfigBatch()
        batch.entries.append(ConfigEntry(key, value))
        self._notify.send(batch)

    def _subscribers_changed(self, number):
        if self._number <= number:
            return

        batch = ConfigBatch()
        for key, value in self.store.items():
            batch.entries.append(ConfigEntry(key, value))
        self._notify.send(batch)


if __name__ == '__main__':

    if len(sys.argv) > 1:
        storefile = sys.argv[1]
    else:
        storefile = None

    loop = echolib.IOLoop()
    client = echolib.Client(name="config")
    loop.add_handler(client)

    manager = ConfigManager(client)

    if storefile is not None and os.path.isfile(storefile):
        manager.read_config(storefile)

    def shutdown_handler(signum, frame):
        if storefile is not None:
            manager.write_config(storefile)
        sys.exit(0)

    signal.signal(signal.SIGTERM, shutdown_handler)

    try:
        while loop.wait(100):
            pass
    except KeyboardInterrupt:
        pass
    finally:
        pass

    if storefile is not None:
        manager.write_config(storefile)

    shutdown_handler(0, None)
