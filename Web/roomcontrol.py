
import os
import jinja2
import webapp2
import json
import urllib
from google.appengine.api import urlfetch
from configuration import imp_keys

JINJA_ENVIRONMENT = jinja2.Environment(
    loader=jinja2.FileSystemLoader(os.path.dirname(__file__)),
    extensions=['jinja2.ext.autoescape'],
    autoescape=True)

agent_url = 'https://agent.electricimp.com/'

class modeType():
    index = 0
    string = " "

    def __init__(self, index, string):
        self.index = index
        self.string = string


mode_strings = [modeType(0, 'CO All'),
                modeType(1, 'CO Spread'),
                modeType(2, 'Rainbow All'),
                modeType(3, 'Rainbow Chase'),
                modeType(4, 'Fixed Color'),
                modeType(5, 'Strobe'),
                modeType(6,  'YO!')]

template_values = {'mode': 0,
                   'pixel_brightness': 255,
                   'strobe_delay': 100,
                   'rainbow_delay': 100,
                   'co_spread_delay': 100,
                   'color_change_cutoff': 750,
                   'min_db': 45.0,
                   'max_db': 65.0,
                   'bass_freq':100,
                   'mid_freq': 600,
                   'treble_freq': 4000}





class homepage(webapp2.RequestHandler):
    def get(self):
        url = agent_url+imp_keys[0]+'/api/getAll'
        try:
        
            result = urlfetch.fetch(url = url,
                             deadline = 10)

            if (result.status_code == 200):
                template_values = json.loads(result.content)
                template_values.update({'mode_strings':mode_strings})
                template = JINJA_ENVIRONMENT.get_template('index_connected.html')
                self.response.write(template.render(template_values))
            else:
                raise
        except Exception, e:
            print e
            template_values = {
                   'connected': False,
                   'mode': 0,
                   'pixel_brightness': 255,
                   'strobe_delay': 100 ,
                   'rainbow_delay': 100,
                   'co_spread_delay': 100,
                   'color_change_cutoff': 750,
                   'min_db': 45.0,
                   'max_db': 65.0,
                   'bass_freq':100,
                   'mid_freq': 600,
                   'treble_freq': 4000}
            template_values.update({'mode_strings':mode_strings})

            # template = JINJA_ENVIRONMENT.get_template('index_disconnected.html')
            template = JINJA_ENVIRONMENT.get_template('index_connected.html')
            self.response.write(template.render(template_values))

class roomInterface(webapp2.RequestHandler):
    def post(self, roomNumber):
        try:
            success = True
            error = ''
            inObj = json.loads(self.request.body)
            base_url = agent_url+imp_keys[eval(roomNumber)]+'/api'

            # keys should params on teensy
            for key in inObj:
                result = urlfetch.fetch(url = base_url +  '/' + key + '?value=' + str(inObj[key]),
                            #payload = inObj[key],
                            method = urlfetch.POST,
                            deadline = 10)

                if (result.status_code != 200):
                    success = False
                    error += result.content + '\n'

	
            if success:
                self.response.headers['Content-Type'] = 'text/plain'
                self.response.status = 200
                self.response.write('ok')
            else:
                self.response.headers['Content-Type'] = 'text/plain'
                self.response.status = 400  ## TODO: status codes handling on client side???
                self.response.write(error)

        except Exception, e:
            self.response.headers['Content-Type'] = 'text/plain'
            self.response.status = 500
            #self.response.write('Error: ' + str(e))
            self.response.write('Error')
            print  str(e)
            
    def get(self, roomNumber):
        try:
            inObj = json.loads(self.request.body)
            base_url = agent_url+imp_keys[eval(roomNumber)]+'/api'

            items = {}

            for key in inObj:
                result = urlfetch.fetch(url = base_url+'/'+key,
                                        method = urlfetch.GET,
                                        deadline = 10)
                if (result.status_code == 200):
                    items.update({key:result.content})

            outObj = json.dumps(items)
            self.response.headers['Content-Type'] = 'application/json'
            self.response.status = 200
            self.response.write(outObj)

        except Exception, e:
            self.response.headers['Content-Type'] = 'text/plain'
            self.response.status = 500
            self.response.write('Error: ' + str(e))

class roomInterfaceRGB(webapp2.RequestHandler):
    def post(self, roomNumber):
        try:
            inObj = json.loads(self.request.body)
            base_url = agent_url+imp_keys[eval(roomNumber)]+'/api/rgb'
            query_url = base_url + '?red=' + str(inObj['r']) + '&green=' + str(inObj['g']) + '&blue=' + str(inObj['b'])
            result = urlfetch.fetch(url = query_url,
                                    method = urlfetch.POST,
                                    deadline = 10)
            if (result.status_code == 200):
                self.response.headers['Content-Type'] = 'text/plain'
                self.response.status = 200
                self.response.write('ok')
            else:
                self.response.headers['Content-Type'] = 'text/plain'
                self.response.status = 400
                self.response.write(result.content)

        except Exception, e:
            self.response.headers['Content-Type'] = 'text/plain'
            self.response.status = 500
            self.response.write('Error: ' + str(e)) 


application = webapp2.WSGIApplication([ 
        ('/', homepage),
        (r'/api/room/(\d+)/rgb', roomInterfaceRGB),
        (r'/api/room/(\d+)', roomInterface),
        #('/api/<room>/')
    ], debug=True)



