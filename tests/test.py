#!/usr/bin/env python
import logging
import logging.handlers
from logging import RootLogger
import sys
import os


LOG_DEBUG = logging.DEBUG
LOG_ERROR = logging.ERROR
LOG_INFO = logging.INFO
LOG_WARN = logging.WARN
LOG_WARNING = logging.WARNING
LOG_CRITICAL = logging.CRITICAL
LOG_FATAL = logging.FATAL
LOG_NOTSET = logging.NOTSET


__default_servicename__ = os.path.splitext(os.path.basename(sys.argv[0]))[0]


class StdoutLogger(object):
    def __init__(self, loglevel=LOG_DEBUG, servicename=__default_servicename__):
        self.loglevel = loglevel
        h = logging.StreamHandler()
        h.setLevel(loglevel)
        fmt = logging.Formatter("%(asctime)s "+servicename+"[%(process)d]: %(levelname)s: %(message)s")
        h.setFormatter(fmt)
        self._logger = RootLogger(loglevel)
        self._logger.addHandler(h)

    def set_debug(self):
        self.loglevel = LOG_DEBUG
        self._logger.setLevel(self.loglevel)

    def set_info(self):
        self.loglevel = LOG_INFO
        self._logger.setLevel(self.loglevel)

    def info(self, msg):
        self._logger.info(str(msg))

    def debug(self, msg):
        self._logger.debug(str(msg))

    def warn(self, msg):
        self._logger.warn(str(msg))

    def error(self, msg):
        self._logger.error(str(msg))

    def write(self, msg):
        self.info(msg)

import sys
res = sys.stdin.readline()
if res.startswith("connected"):
    print "yes\n"
    #sys.stdout.write("yes\n")
    #sys.stdout.flush()
else:
    print "no\n"
    #sys.stdout.write("no\n")
    #sys.stdout.flush()
