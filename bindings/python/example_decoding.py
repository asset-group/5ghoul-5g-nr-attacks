#!/usr/bin/env python3

from binascii import hexlify, unhexlify
from wdissector import \
    Machine, wd_init, wd_field, wd_filter, wd_read_filter, wd_register_filter, packet_read_field_uint64, \
    wd_register_field, wd_packet_dissect, wd_packet_dissectors, wd_packet_layers_count, wd_read_field, \
    wd_packet_show, wd_packet_show_pdml, wd_info_profile, wd_packet_summary, packet_read_value_to_string, \
    wd_set_packet_direction, wd_set_dissection_mode, packet_read_field_string, packet_read_field_display_name, \
    WD_DIR_TX, WD_DIR_RX, WD_MODE_NORMAL, WD_MODE_FAST, WD_MODE_FULL
from scapy.utils import rdpcap
from scapy.packet import raw
from colorama import Fore, Back, Style
import colorama

colorama.init(autoreset=True)

# ------------------ Sample Packets ------------------
pkts = rdpcap("captures/capture_dialog_DA14680_truncated_l2cap_crash.pcap")

interesting_pkts = []
interesting_pkts.append((WD_DIR_TX, pkts[58]))
interesting_pkts.append((WD_DIR_RX, pkts[60]))
interesting_pkts.append((WD_DIR_TX, pkts[64]))
interesting_pkts.append((WD_DIR_RX, pkts[66]))
interesting_pkts.append((WD_DIR_TX, pkts[67]))

# ------------------ WDissector Initialization ------------------
print('\n---------------------- WDissector -----------------------')
# Initialize protocol
wd = wd_init("encap:NORDIC_BLE")  # or encap:NORDIC_BLE
# Declare Filters
filter_string = "btle.data_header.length >= uint(frame[23:2]) + 4"
f1 = wd_filter(filter_string)
if not f1:
    print("Filter not valid")
    exit(1)
# Declare Fields to be read
fd1 = wd_field("btle.control_opcode")  # Link Layer Opcode
if not fd1:
    print("Field not valid")
    exit(1)
fd2 = wd_field("btatt.opcode.method")  # L2CAP Opcode
if not fd2:
    print("Field not valid")
    exit(1)

print(f'{Fore.GREEN}WDissector fully initialized!')


# ------------------ WDissector Packet Decoding ------------------
for dir, pkt in interesting_pkts:
    print('---------------------------------------------------------')
    pkt = bytearray(raw(pkt))  # Convert to raw and then to bytearray
    # 1) Register filters and fields here if any (this needs to be done before any call to wd_packet_dissect)
    wd_register_filter(wd, f1)
    wd_register_field(wd, fd1)
    wd_register_field(wd, fd2)

    # 2) Set packet direction (WD_DIR_TX or WD_DIR_RX) and decode packet
    wd_set_packet_direction(wd, dir)
    wd_packet_dissect(wd, pkt, len(pkt))

    # 3) Read packet filter (True or False) and field results (ptr or None)
    filter_result = wd_read_filter(wd, f1) # Returns True or False
    opcode_ll = wd_read_field(wd, fd1) # Returns pointer to field or None
    opcode_l2cap = wd_read_field(wd, fd2) # Returns pointer to field or None

    # 4) Print packet summary and filter result
    packet_summary = wd_packet_summary(wd)

    print(f'{Fore.YELLOW}Raw Packet:{hexlify(pkt)}, Length={len(pkt)}')
    if dir == WD_DIR_TX:
        print(f'{Fore.CYAN}TX --> {packet_summary}')
    else:
        print(f'{Fore.GREEN}RX <-- {packet_summary}')

    print(
        f'Packet Layers: {wd_packet_layers_count(wd) - 1} ({wd_packet_dissectors(wd)})')
    color = Fore.GREEN if filter_result else Fore.RED
    
    # check if L2CAP opcode field exists
    if opcode_l2cap:
        value = packet_read_field_uint64(opcode_l2cap)
        opcode_text = packet_read_value_to_string(value, fd2)
        print(f'L2CAP Opcode: {opcode_text} ({value})')
    # check if LL opcode field exists
    elif opcode_ll:
        value = packet_read_field_uint64(opcode_ll) # Read integer value from field
        opcode_text = packet_read_value_to_string(value, fd1) # Read text from field dictionary
        print(f'LL Opcode: {opcode_text} ({value})')

    # Check filter condition only for l2cap packets
    if not opcode_ll:
        print(f'{color}--> Condition Result: {filter_result} (\'{filter_string}\')')
    
    dir_str = "TX" if dir == WD_DIR_TX else "RX"
    layer_str = "L2CAP" if opcode_l2cap else "LL Ctrl"
    state_label = f'{dir_str} / {layer_str} / {opcode_text}'
    print(f'Simple Packet Label: {state_label}')
