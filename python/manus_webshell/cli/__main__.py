import sys
import os
import json
import errno
import glob
from urllib.request import urlopen, build_opener, Request
import argparse
import mimetypes

def mkdir_p(path):
    try:
        os.makedirs(path)
    except OSError as exc:  # Python >2.5
        if exc.errno == errno.EEXIST and os.path.isdir(path):
            pass
        else:
            raise

def do_backup(args):
    url = "http://%s/api/storage" % args.host
    response = urlopen(url)
    data = response.read()
    keys = json.loads(data)
    mkdir_p(args.storage)
    for key in keys:
        response = urlopen("%s?key=%s" % (url, key))
        data = response.read()
        type = mimetypes.guess_extension(response.info().get_content_type())
        print("Saving %s" % key)
        with open(os.path.join(args.storage, "%s%s" % (key, type)), "w") as file:
            file.write(data.decode("utf-8"))

def do_clear(args):
    url = "http://%s/api/storage" % args.host
    response = urlopen(url)
    data = response.read()
    keys = json.loads(data)
    for key in keys:
        response = urlopen("%s?key=%s" % (url, key), "".encode("utf-8"))
        data = response.read()
        print("Deleting %s" % key)

def do_restore(args):
    url = "http://%s/api/storage" % args.host
    for element in glob.glob(os.path.join(args.storage, '*')):
        (key, ext) = os.path.splitext(os.path.basename(element))
        (ctype, encoding) = mimetypes.guess_type(element)
        with open(element, "r") as file:
            data = file.read()
            opener = build_opener()
            request = Request("%s?key=%s" % (url, key), data=data.encode("utf-8"),
                 headers={'Content-Type': ctype})
            response = opener.open(request)
            print("Restoring %s" % key)


def do_run(args):
    url = "http://%s/api/apps" % args.host
    
    with open(args.file, "r") as file:
        data = file.read()
        opener = build_opener()
        request = Request(url, data=json.dumps({"code" : data}),
                headers={'Content-Type': 'application/json'})
        response = opener.open(request)
        print("Running %s" % args.file)

if len(sys.argv) < 2:
    sys.exit(1)

parser = argparse.ArgumentParser(description='Manus API CLI utility', prog=sys.argv[0])

parser.add_argument('--host', dest='host', help='API server address and port', default="localhost:8080")

if sys.argv[1] == 'backup':
    parser.add_argument('-s', dest='storage', help='Local storage directory', default=os.path.join(os.getcwd(), 'manus_storage'))
    operation = do_backup

if sys.argv[1] == 'restore':
    parser.add_argument('-s', dest='storage', help='Local storage directory', default=os.path.join(os.getcwd(), 'manus_storage'))
    operation = do_restore

if sys.argv[1] == 'clear':
    operation = do_clear

if sys.argv[1] == 'run':
    parser.add_argument('file', metavar='file', type=str, help='Python script to upload and run')
    operation = do_run

args = parser.parse_args(sys.argv[2:])

operation(args)


