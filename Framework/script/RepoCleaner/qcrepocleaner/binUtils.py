#!/usr/bin/env python3

import logging
import sys


def prepare_main_logger():
    logger = logging.getLogger()
    # Logging (split between stderr and stdout)
    formatter = logging.Formatter(fmt='%(asctime)s - %(levelname)s - %(message)s', datefmt='%d-%b-%y %H:%M:%S')
    h1 = logging.StreamHandler(sys.stdout)
    h1.setLevel(logging.DEBUG)
    h1.addFilter(lambda record: record.levelno <= logging.INFO) # filter out everything that is above INFO level
    h1.setFormatter(formatter)
    logger.addHandler(h1)
    h2 = logging.StreamHandler(sys.stderr)
    h2.setLevel(logging.WARNING)     # take only warnings and error logs
    h2.setFormatter(formatter)
    logger.addHandler(h2)