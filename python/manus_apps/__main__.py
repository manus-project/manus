from __future__ import absolute_import

import sys, os

if __name__ == '__main__':
    from .launcher import application_launcher
    application_launcher(sys.argv[1] if len(sys.argv) > 1 else None)
