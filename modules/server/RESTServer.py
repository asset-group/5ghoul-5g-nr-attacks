#!/usr/bin/env python3

import pyximport
pyximport.install()
import eventlet
# Patch standard libraries to use eventlet
eventlet.monkey_patch()
import socket
import os
import logging
from eventlet.greenio import GreenSocket
from flask import Flask
from flask import request
from flask_cors import CORS
from flask_selfdoc import Autodoc


def EventletWorker(func):
    ''' Decorator to run function on eventlet thread '''

    def change_context(*args):
        cls = args[0]  # type: RESTServer
        if hasattr(cls, 'worker_fcns'):
            cls.worker_fcns.append((func, *args))
            cls.s1.sendall(b'\xFF')
        return None

    return change_context


class RESTServer(object):
    ''' Basic Flask REST Server. No CRUD operations are done '''

    namespace = None
    logger = False
    app = None  # type: Flask
    autodoc = None  # type: Autodoc
    port = None
    host = None
    running = False
    clients = 0  # This is always 0 for REST, clients do not stay connected after URL request

    handler = None
    worker_fcns = []
    s1 = None  # Socket of thread end
    s2 = None  # Socket of eventlet end

    def __init__(self, namespace='/'):
        self.namespace = namespace
        # Create socket for IPC communication
        self.s1, self.s2 = socket.socketpair()
        self.s1.setblocking(True)
        self.s2.setblocking(True)
        self.s2 = GreenSocket(self.s2)

        # Initialize Flask server app instance
        self.app = Flask(__name__)
        # Enable CORS for all routes
        CORS(self.app)
        # Disable warning banner
        os.environ["WERKZEUG_RUN_MAIN"] = "true"

        # Add auto documentation to flask app exposed on namespace
        self.autodoc = Autodoc(self.app)
        self.app.add_url_rule(namespace, None, self.api)

    def _server_handler(self):
        ''' Main server thread '''
        self.running = True

        try:
            eventlet.spawn(self._server_worker)
            self.app.run(host=self.host, port=self.port)
        except eventlet.greenlet.GreenletExit:
            # Silently exit server thread
            pass

    def _server_worker(self):
        ''' Worker thread that execute functions calls to this class from another thread '''
        while True:
            self.s2.recv(1, socket.MSG_WAITALL)
            while len(self.worker_fcns) > 0:
                fcn, *args = self.worker_fcns.pop(0)
                fcn(*args)

    def register_callback(self, fcn_name, fcn,
                          description=None):
        ''' Register external callback (from c++ user code) '''

        # connect and disconnect callbacks are only handled in SocketIO
        if fcn_name == 'connect' or fcn_name == 'disconnect':
            return

        # Generate callback function entry point (prepare arguments to user fcn)
        # @self.autodoc.doc()
        def gen_fcn():
            gen_fcn.__doc__ = "Oh My god"
            args = []
            data = None

            if request.method == 'GET':
                if 'data' in request.args:
                    data = request.args.get('data')
            elif request.method == 'POST':
                if 'data' in request.form.keys():
                    data = request.form['data']
                else:
                    data = request.get_data(cache=False,
                                            as_text=True,
                                            parse_form_data=False)

            if data != None and len(data):
                args.append(data)

            # return result of C++ callback function
            return fcn(args)

        # Add description to function if available
        gen_fcn.__doc__ = description

        # Inserts wraps gen_fcn into autodoc
        gen_fcn = self.autodoc.doc()(gen_fcn)
        self.app.add_url_rule(self.namespace + str(fcn_name),
                              fcn_name, gen_fcn,
                              methods=['GET', 'POST'])

    def start(self, port, host='0.0.0.0', logger=False):
        ''' Start Rest Server '''
        if self.running is False:
            self.clients = 0
            self.port = port
            self.host = host
            self.logger = logger

            log = logging.getLogger('werkzeug')
            if self.logger is False:
                # Disable logger
                log.setLevel(logging.WARNING)
            else:
                log.setLevel(logging.DEBUG)

            self.handler = eventlet.spawn(self._server_handler)
            self.handler.wait()
            self.running = False

    def api(self):
        ''' Return autodoc API documentation HTML page '''
        return self.autodoc.html(title='REST API Documentation', template="custom_documentation.html")

    @EventletWorker
    def stop(self):
        ''' Stop server by thowing eventlet thread '''
        if self.handler:
            self.handler.throw()

    @EventletWorker
    def disconnect_all(self):
        ''' Force diconnection of all clients (Not applicable to REST) '''
        # REST does not have disconnect
        pass

    @EventletWorker
    def send_event(self, event, msg):
        ''' Send events to all clients (Not applicable to REST) '''
        # Rest cannot send events by itself
        # TODO: Simply do a URL POST request containing the msg to the client here
        # It would be up to the receiving client to implement a server which receives such post requests
        # For example, GitLab trigger build notifications via their Web Hooks system, which sends data
        # to an user defines address
        pass
