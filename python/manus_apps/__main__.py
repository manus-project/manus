from __future__ import absolute_import

import sys, os

if __name__ == '__main__':
	if len(sys.argv) < 2:
	    from .launcher import application_launcher
	    application_launcher()
	else:
		from .launcher import start_app
		start_app(sys.argv[1])