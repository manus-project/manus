

from manus.messages import ConfigBatch, ConfigEntry, ConfigEntryPublisher


class Handler(object):

    def handle(self, arguments):
        raise NotImplementedError()

    def run(self):
        pass

    def stop(self):
        pass


class ConfigPublisher(object):

    def __init__(self, client, name, topic="config.update"):
        self._publisher = ConfigEntryPublisher(client, topic)
        self._name = name
        self._current = None

    def __call__(self, value):
        if self._current == value:
            return

        self._current = value
        self._publisher.send(ConfigEntry(self._name, value))
        
