#!/usr/bin/python

import os, sys
from manus_starter import run_service, run_interactive

MANUS_LAUNCHFILES = os.getenv('MANUS_LAUNCHFILES', '/usr/share/manus/launch/virtual/')

if __name__ == '__main__':

    if len(sys.argv) < 2:
        run_interactive(MANUS_LAUNCHFILES)
    else:
        run_service(sys.argv[1])

