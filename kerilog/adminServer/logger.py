import logging
import os

LOG_FN = 'mqtt'
LOG_PATH = '/var/log/m2mqtt/'
HANDLERS = [
    logging.FileHandler("{0}/{1}.log".format(LOG_PATH, LOG_FN)),
]
if "DEBUG" in os.environ:
    LOG_LEVEL = logging.DEBUG
    HANDLERS.append(logging.StreamHandler())
else:
    LOG_LEVEL = logging.WARNING

# setup for global logger
def basicLogger():
    logging.basicConfig(
        format="%(asctime)s[%(levelname)-5.5s]  %(message)s",
        handlers=HANDLERS,
        level=LOG_LEVEL
    )

# setup fromBridge Logger
def bridgeLogger():
    customLogger("bridge_mqtt", "fromBridge")

# setup fromNode Logger
def nodeLogger():
    customLogger("node_mqtt", "fromNode")

def customLogger(logger_name, file_name):
    l = logging.getLogger(logger_name)
    formatter = logging.Formatter("%(asctime)s[%(levelname)-5.5s]  %(message)s")
    fileHandler = logging.FileHandler("{0}/{1}.log".format(LOG_PATH, file_name))
    fileHandler.setFormatter(formatter)
    streamHandler = logging.StreamHandler()
    streamHandler.setFormatter(formatter)
    l.setLevel(LOG_LEVEL)
    l.addHandler(fileHandler)
    l.addHandler(streamHandler)

basicLogger()
bridgeLogger()
nodeLogger()
