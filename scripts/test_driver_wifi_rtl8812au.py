#!/usr/bin/env python

import socket
import struct
import binascii
import os
import signal
import ctypes
import colorama
from scapy.layers.dot11 import RadioTap, Dot11, Dot11Beacon, Dot11ProbeResp, Dot11ProbeReq, Dot11TKIP, LLC
from scapy.utils import PcapWriter
from scapy.packet import Raw, raw
from threading import Thread
from wdissector import *
from colorama import Fore

# driver netlink commands
NETLINK_CMD_SELECT_DEVICE = 0
NETLINK_CMD_READ_ADDR = 1
NETLINK_CMD_WRITE_ADDR = 2
NETLINK_CMD_MULTIWRITE_ADDR = 3
NETLINK_CMD_MULTIREAD_ADDR = 4
NETLINK_CMD_INTERRUPT_RX_ENABLE = 5
NETLINK_CMD_INTERCEPTION_TX_ENABLE = 6
NETLINK_CMD_FORCE_FLAGS_ENABLE = 7
NETLINK_CMD_FORCE_FLAGS_RETRY = 8
NETLINK_CMD_SEND_DATA = 9
NETLINK_CMD_INTERCEPT_TX = 10
# driver netlink events to user-space
NETLINK_EVT_TX = 0
NETLINK_EVT_RX = 1

# rt2080/3080 registers
MAC_ADDR_DW0 = 0x1008
MAC_ADDR_DW1 = 0x100C
MAC_BSSID_DW0 = 0x1010
MAC_BSSID_DW1 = 0x1014

RX_FILTER_CFG = 0x1400
AUTO_RSP_CFG = 0x1404


def thread_dhcp():
    os.system("ifconfig wlan1 up 192.168.42.1 netmask 255.255.255.0")
    os.system("iptables -I INPUT -i wlan1 -j ACCEPT")
    os.system(
        "iptables -t nat -A POSTROUTING -s 192.168.42.0/24 ! -o wlan1 -j MASQUERADE")
    os.system("dnsmasq --no-daemon --bind-interfaces --interface=wlan1 --dhcp-range=192.168.42.2,192.168.42.127,24h")


class RT2800USBNetlink:
    # Default Parameters
    NETLINK_USER = 25
    NETLINK_PID = None
    NETLINK_GROUP = 1
    NETLINK_BUFFER_SIZE = 1000000
    n_debug = False
    # Internal vars
    n_socket = None
    stop_request = False

    # Constructor ------------------------------------
    def __init__(self, debug=n_debug, mac_addr=0x00, retry_enable=0, filter_unicast=0, filter_control=0,
                 netlink_port=None, netlink_pid=None, netlink_group=None, netlink_buffer=None):

        # Set default vars
        if netlink_port is not None:
            self.NETLINK_USER = netlink_port

        if netlink_pid is None:
            self.NETLINK_PID = os.getpid()
        else:
            self.NETLINK_PID = netlink_pid

        if netlink_group is not None:
            self.NETLINK_GROUP = netlink_group

        if netlink_buffer is not None:
            self.NETLINK_BUFFER_SIZE = netlink_buffer

        if debug is not None:
            self.n_debug = debug

        self.n_socket = socket.socket(
            socket.AF_NETLINK, socket.SOCK_RAW, self.NETLINK_USER)
        self.n_socket.setsockopt(
            socket.SOL_SOCKET, socket.SO_SNDBUF, self.NETLINK_BUFFER_SIZE)
        self.n_socket.setsockopt(
            socket.SOL_SOCKET, socket.SO_RCVBUF, self.NETLINK_BUFFER_SIZE)
        self.n_socket.bind((self.NETLINK_PID, self.NETLINK_GROUP))

        self.select_device(0)

        # Use default device_id=0
        if (self.n_debug):
            print('RT2800USBNetlink: Instance started')

    def close(self):
        self.stop_request = True
        self.n_socket.close()
        if (self.n_debug):
            print('RT2800USB Driver closed')

    # RAW socket functions ---------------------------
    def raw_send(self, data):
        if self.n_debug:
            print('Bytes sent: ' + binascii.hexlify(data))
        self.n_socket.send(data)

    def raw_receive(self):
        data = self.n_socket.recv(self.NETLINK_BUFFER_SIZE)
        if self.n_debug:
            print(str(len(data)) + ' bytes received')
            print("Hex: " + binascii.hexlify(data[::-1]))
            print("Int: " + str(struct.unpack("<L", data)[0]))
        return data

    # Netlink Comands -------------------------------------

    def select_device(self, dev_id=0):
        if self.n_debug:
            print('\nSELECT command')
        netlink_data = struct.pack('<BLL', NETLINK_CMD_SELECT_DEVICE,
                                   os.getpid(), dev_id)
        self.raw_send(netlink_data)
        res = self.raw_receive()

        return dev_id

    def read(self, addr):
        if self.n_debug:
            print('\nREAD command')
        netlink_data = struct.pack('<BL', NETLINK_CMD_READ_ADDR, addr)
        self.raw_send(netlink_data)
        data = self.raw_receive()  # Get reading
        return data

    def write(self, addr, value, size=0):
        if self.n_debug:
            print('\nWRITE command')
        if size == 0:  # normal write
            netlink_data = struct.pack('<BLL', NETLINK_CMD_WRITE_ADDR,
                                       addr, value)
        else:
            netlink_data = struct.pack('<BLB' + str(size) + 's', NETLINK_CMD_MULTIWRITE_ADDR,
                                       addr, size, value)  # value here is in bytes

        self.raw_send(netlink_data)
        data = self.raw_receive()  # Get status
        return data

    def send_data(self, data):
        netlink_data = chr(NETLINK_CMD_SEND_DATA) + \
            data  # value here is in bytes
        self.n_socket.send(netlink_data)

    def set_mac(self, mac):
        netlink_data = ''.join((mac.split(':'))).decode('hex')
        data = self.write(MAC_ADDR_DW0, netlink_data, size=6)
        return data

    def set_flags_enable(self, value):
        netlink_data = struct.pack(
            '<BB', NETLINK_CMD_FORCE_FLAGS_ENABLE, value)
        self.raw_send(netlink_data)
        if self.n_debug:
            print('force flags set to ' + str(value))
        return self.raw_receive()

    def set_flags_retry(self, value):
        netlink_data = struct.pack('<BB', NETLINK_CMD_FORCE_FLAGS_RETRY, value)
        self.raw_send(netlink_data)
        if self.n_debug:
            print('retry flags set to ' + str(value))
        return self.raw_receive()

    def set_filter(self, value):
        self.write(RX_FILTER_CFG, value)

    def set_filter_sniffer(self):
        self.write(AUTO_RSP_CFG, 0x0007)
        self.write(RX_FILTER_CFG, 0x0093)

    def set_filter_unicast(self):
        self.write(RX_FILTER_CFG, 0x1BF97)
        #self.write(RX_FILTER_CFG, 0x1BFB7)
        self.write(AUTO_RSP_CFG, 0x0017)

    def set_filter_unicast_only(self):
        self.write(RX_FILTER_CFG, 0x1BFD7)
        self.write(RX_FILTER_CFG, 0x1BFE7)
        #self.write(AUTO_RSP_CFG, 0x0017)

    def set_auto_rsp(self, value):
        self.write(AUTO_RSP_CFG, value)

    def set_interrupt_rx_enable(self):
        netlink_data = struct.pack('<BB', NETLINK_CMD_INTERRUPT_RX_ENABLE, 1)
        self.raw_send(netlink_data)
        if self.n_debug:
            print('RX Interrupt set to 1')
        return self.raw_receive()

    def set_interception_tx(self, value):
        netlink_data = struct.pack('<BB',
                                   NETLINK_CMD_INTERCEPTION_TX_ENABLE, value)
        self.raw_send(netlink_data)
        if self.n_debug:
            print('TX Interception set to ' + str(value))
        return self.raw_receive()

    def set_interrupt_rx_disable(self):
        netlink_data = struct.pack('<BB', NETLINK_CMD_INTERRUPT_RX_ENABLE, 0)
        self.raw_send(netlink_data)
        if self.n_debug:
            print('RX Interrupt set to 0')
        return self.raw_receive()

    def intercept_tx(self):
        netlink_data = struct.pack('<B', NETLINK_CMD_INTERCEPT_TX)
        self.raw_send(netlink_data)


# ------------ DEMO --------------------
colorama.init(autoreset=True)

RT2800 = RT2800USBNetlink(debug=True)
# RT2800.set_mac('00:00:00:00:00:00')
# RT2800.set_filter_unicast()
# RT2800.set_filter_sniffer()
RT2800.set_interrupt_rx_enable()
# RT2800.set_interception_tx(1)
# RT2800.set_interrupt_rx_disable()
# RT2800.set_flags_enable(1)
# RT2800.set_flags_retry(1)
# RT2800.read(MAC_ADDR_DW0)
# RT2800.read(MAC_ADDR_DW1)
# RT2800.read(RX_FILTER_CFG)
wdissector_init("encap:IEEE802_11_RADIO")

print("Version: " + wdissector_version_info().decode())
print("Loaded Profile: " + wdissector_profile_info().decode())

RT2800.n_debug = False
t = Thread(target=thread_dhcp)
t.daemon = True
t.start()


pktdump = PcapWriter("capture.pcapng", sync=False)

# try:

while True:
    data = RT2800.raw_receive()
    #print(str(len(data)) + " Hex: " + binascii.hexlify(data))
    nl_evt = ord(data[0])

    if nl_evt == NETLINK_EVT_TX:
        # RT2800.intercept_tx()
        pass

    d = Dot11(data[1:])
    pkt_raw = str(Raw(RadioTap() / d))
    pkt_raw = [ord(x) for x in pkt_raw]

    # if Dot11Beacon not in d and Dot11TKIP not in d and Dot11ProbeReq not in d:
    if Dot11Beacon in d:
        print(binascii.hexlify(raw(RadioTap())))
        print((Fore.CYAN + "TX --> " if nl_evt ==
               NETLINK_EVT_TX else Fore.GREEN + "RX <-- ") + d.summary())
        pkt = (ctypes.c_ubyte * len(pkt_raw))(*pkt_raw)
        packet_dissect(pkt, len(pkt))
        print(packet_summary().decode())
        pktdump.write(d)

    # if Dot11TKIP in d:
    #  # d.show()
    #  print(LLC(d[Dot11TKIP].data).summary())
    # print(d.summary())
    # print(len(data))

# except Exception as e:
#     print(Fore.RED + str(e))
#     print('\nClosing')
#     RT2800.close()
#     pktdump.close()
#     os.kill(os.getpid(), signal.SIGTERM)
