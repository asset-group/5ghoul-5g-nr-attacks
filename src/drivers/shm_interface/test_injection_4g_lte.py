#!/usr/bin/env python3

import zmq, json
import os, sys, io, signal
from time import sleep
from threading import Thread

os.system('clear')

req_addr = "tcp://localhost:5563"
fuzz_duplicate = dict()
global flag_update_duplicate

test_buf = [0x21, 0x02, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]

def new_duplicate(sdu_len=0, pdu_buf=list()):
    fuzz_duplicate = {'sdu_len':sdu_len, 'pdu_buf':pdu_buf}
    sjson_duplicate = json.dumps(fuzz_duplicate)
    return sjson_duplicate


## request thread
def handle_req_thread(context, ): 
    print("Connecting to rep server…")
    req_sock = context.socket(zmq.REQ)
    req_sock.connect(req_addr)
    print("Connected to rep server")

    flag_update_duplicate = True

    while True:
        if flag_update_duplicate: 
            req_str = new_duplicate(sdu_len=2, pdu_buf=test_buf)   
            print("req_duplicate:", req_str);
            req_sock.send(req_str.encode('utf-8'))
            rcv_str = req_sock.recv()

            print("rcv_duplicate:", rcv_str);
            if rcv_str == b"update_succeed":
                flag_update_duplicate = False

        sleep(0.1)

    req_sock.close()



if __name__ == '__main__':
    
    try:
        signal.signal(signal.SIGINT, quit)
        signal.signal(signal.SIGTERM, quit)

        context = zmq.Context()
        
        # request thread
        req_thread = Thread(target=handle_req_thread, args=(context, ))
        req_thread.start()

    except Exception: 
        context.term()
        print()
