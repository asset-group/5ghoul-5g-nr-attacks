#!/usr/bin/env python3

import pyximport
pyximport.install()
import eventlet
# Patch standard libraries to use eventlet
eventlet.monkey_patch()
import socketio
import socket
from eventlet.greenio import GreenSocket


def EventletWorker(func):
    # Decorator to run function on eventlet thread
    def change_context(*args):
        cls = args[0]  # type: SocketIOServer
        if hasattr(cls, 'worker_fcns'):
            cls.worker_fcns.append((func, *args))
            cls.s1.sendall(b'\xFF')
        return None

    return change_context


class SocketIOServer(socketio.Namespace):
    namespace = None
    logging = False
    sio = None
    app = None
    port = None
    host = None
    running = False
    clients = 0
    clients_list = []

    handler = None
    worker_fcns = []
    s1 = None  # Socket of thread end
    s2 = None  # Socket of eventlet end

    def __init__(self, namespace='/'):
        super().__init__(namespace=namespace)
        self.namespace = namespace
        # Create socket for IPC communication
        self.s1, self.s2 = socket.socketpair()
        self.s1.setblocking(True)
        self.s2.setblocking(True)
        self.s2 = GreenSocket(self.s2)

    def _server_handler(self):
        self.running = True
        try:
            eventlet.spawn(self._server_worker)
            eventlet.wsgi.server(eventlet.listen((self.host, self.port)), self.app,
                                 log_output=self.logging)
        except eventlet.greenlet.GreenletExit:
            # Silently exit server thread
            pass

    def _server_worker(self):
        while True:
            self.s2.recv(1, socket.MSG_WAITALL)
            while len(self.worker_fcns) > 0:
                fcn, *args = self.worker_fcns.pop(0)
                fcn(*args)

    def register_callback(self, fcn_name, fcn, *args):
        # Generate callback function
        if fcn_name == 'connect':
            def gen_fcn(sid, *args):
                self.clients_list.append(sid)
                return fcn(list(*args))

        elif fcn_name == 'disconnect':
            def gen_fcn(sid, *args):
                if sid in self.clients_list:
                    self.clients_list.remove(sid)
                return fcn(list(*args))

        else:
            def gen_fcn(sid, *args):
                return fcn(list(args))

        self.__setattr__('on_' + fcn_name, gen_fcn)

    def start(self, port, host='0.0.0.0', logging=False):
        if self.running is False:
            self.clients = 0
            self.port = port
            self.host = host
            self.logging = logging

            self.sio = socketio.Server(async_mode='eventlet', cors_allowed_origins='*',
                                       logger=logging, engineio_logger=logging)
            self.sio.register_namespace(self)
            self.app = socketio.WSGIApp(self.sio)
            self.handler = eventlet.spawn(self._server_handler)
            self.handler.wait()
            self.running = False

    @EventletWorker
    def stop(self):
        if self.handler:
            self.handler.throw()

    @EventletWorker
    def disconnect_all(self):
        for sid in self.clients_list:
            self.sio.disconnect(sid)
        self.clients_list = []

    @EventletWorker
    def send_event(self, event, msg):
        if self.sio is not None and len(self.clients_list):
            self.sio.emit(event, msg)
