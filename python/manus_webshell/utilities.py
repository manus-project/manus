
import os
import thread
import json
import numpy

def synchronize(f):
    def wrapper(*args, **kwargs):
        obj = args[0]
        if not hasattr(obj, "loop"):
            #print "Warning"
            f(*args, **kwargs)
        else:
            obj.loop.add_callback(f, *args, **kwargs)

    return wrapper

def enum(*sequential, **named):
    enums = dict(zip(sequential, range(len(sequential))), **named)
    reverse = dict((value, key) for key, value in enums.iteritems())
    enums['reverse_mapping'] = reverse
    return type('Enum', (), enums)

try:
  import tornado.ioloop
  import tornado.web

  class RedirectHandler(tornado.web.RequestHandler):

      def __init__(self, application, request, url):
          super(RedirectHandler, self).__init__(application, request)
          self.url = url

      def get(self):
          self.redirect(self.url, True)
          #self.finish()

  class DevelopmentStaticFileHandler(tornado.web.StaticFileHandler):
      def set_extra_headers(self, path):
          # Disable cache
          self.set_header('Cache-Control', 'no-store, no-cache, must-revalidate, max-age=0')

  class NumpyEncoder(json.JSONEncoder):
      def default(self, obj):           
        if isinstance(obj, numpy.integer):
            return int(obj)
        elif isinstance(obj, numpy.floating):
            return float(obj)
        elif isinstance(obj, numpy.ndarray):
            return obj.tolist()
        else:
            return super(NumpyEncoder, self).default(obj)

  class JsonHandler(tornado.web.RequestHandler):
      """Request handler where requests and responses speak JSON."""
      def prepare(self):
          # Incorporate request JSON into arguments dictionary.
          if self.request.body:
              try:
                  json_data = json.loads(self.request.body)
                  self.request.arguments.update(json_data)
              except ValueError:
                  message = 'Unable to parse JSON.'
                  self.send_error(400, message=message) # Bad Request
   
          # Set up response dictionary.
          self.response = dict()

      def set_default_headers(self):
          self.set_header('Content-Type', 'application/json')
          self.set_header('Cache-Control', 'no-store, no-cache, must-revalidate, max-age=0')
   
      def write_error(self, status_code, **kwargs):
          print kwargs
          if 'message' not in kwargs:
              if status_code == 405:
                  kwargs['message'] = 'Invalid HTTP method.'
              else:
                  kwargs['message'] = 'Unknown error.'
       
          #self.response = kwargs
          self.write_json()
       
      def write_json(self):
          output = json.dumps(self.response, cls=NumpyEncoder)
          self.write(output)

except ImportError:
  pass

