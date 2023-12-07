#!/usr/bin/env bash
"exec" "`dirname $0`/../modules/python/install/bin/python3" "$0" "$@"

# IMPORTANT: This script is executed from the root folder of the project so it can use the standalone python3 from wdissector
# Example: ./scripts/server_test.py -a 127.0.0.1 -t Shutdown

import sys
import getopt
import socketio


def PrintHelp():
    print('server_test.py -s <server type> -r <request name> -a <server address> -p <server port> -d <test data>')
    print('server_test.py -s <server type> -w <subscription name> -a <server address> -p <server port> -d <test data>')


def TestSocketIORequest(logger, server_address, server_port, event_name, test_data):
    sio = socketio.Client()

    if test_data:
        logger('Data: ' + test_data)

    @sio.event
    def connect():
        logger('Connection established')
        # event_name is the socketio event name
        logger('Requesting event ' + event_name +
               ' from server ' + server_address + ':' + server_port)
        res = sio.call(event_name, test_data)
        # res is the response message from the server if there is any
        logger(res)
        sio.disconnect()
        logger('Done, disconnecting...')
        sys.exit(0)

    logger(server_address + ':' + server_port)
    try:
        sio.connect(server_address + ':' + server_port, transports='websocket')
    except socketio.exceptions.ConnectionError as err:
        print('Connection error, check if server address/port is correct!')
    try:
        sio.wait()
    except KeyboardInterrupt:
        print('\nTest Interrupted')
    sys.exit(0)


def TestSocketIOSubscription(logger, server_address, server_port, event_name, test_data):
    sio = socketio.Client()

    def receive_event(msg):
        logger('Received from server:')
        logger(msg)

    @sio.event
    def connect():
        logger('Connection established')
        logger('Press Ctrl+C to disconnect and exit')
        logger('Wait event ' + event_name + ' from server ' +
               server_address + ':' + server_port)
        # event_name is the socketio event name
        sio.on(event_name, receive_event)

    logger(server_address + ':' + server_port)
    try:
        sio.connect(server_address + ':' + server_port, transports='websocket')
    except socketio.exceptions.ConnectionError as err:
        print('Connection error, check if server address/port is correct!')
    try:
        sio.wait()
    except KeyboardInterrupt:
        print('\nTest Interrupted')
    sys.exit(0)


def ServerTest(argv):
    # Default options
    server_type = 'SocketIO'
    test_type = 'r'
    test_name = 'GetModelConfig'
    server_address = 'ws://127.0.0.1'
    server_port = '3000'
    test_data = None

    def LogHelper(msg):
        if msg:
            print(('[%s] ' + str(msg)) % (server_type))

    try:
        opts, args = getopt.getopt(
            argv, 'hs:r:w:a:p:d:', ['server=', 'test=', 'address=', 'port=', 'data='])
    except getopt.GetoptError:
        print('Arguments error')
        PrintHelp()
        sys.exit(2)

    if len(opts) < 1:
        print("Assuming default --server=SocketIO --request=GetConfig --adress=127.0.0.1 --port=3000")

    for opt, arg in opts:
        if opt in ("-h", "--help"):
            PrintHelp()
            sys.exit()
        elif opt in ("-s", "--server"):
            server_type = arg
        elif opt in ("-r", "--request"):
            test_type = 'r'
            test_name = arg
        elif opt in ("-w", "--wait"):
            test_type = 'w'
            test_name = arg
        elif opt in ("-d", "--data"):
            test_data = arg
        elif opt in ("-a", "--address"):
            if ('ws://' not in arg) and ('http://' not in arg):
                server_address = 'ws://' + arg
            else:
                server_address = arg
        elif opt in ("-p", "--port"):
            server_port = int(arg)
            pass
        else:
            print('Arguments error')

    if server_type == 'SocketIO':
        if test_type == 'r':
            TestSocketIORequest(LogHelper, server_address,
                                server_port, test_name, test_data)
        elif test_type == 'w':
            TestSocketIOSubscription(LogHelper, server_address,
                                     server_port, test_name, test_data)


if __name__ == "__main__":
    ServerTest(sys.argv[1:])
