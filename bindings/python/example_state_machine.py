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

# ------------------ State Machine Initialization ------------------
print('\n\n--------------------- State Machine ---------------------')
StateMachine = Machine()

ret = StateMachine.init("configs/ble_config.json") # Load State Mapper configuration
if not ret:
    print("Error initializing state machine")
    exit(1)
# Get wdissector instance from state machine initialization
wd = StateMachine.wd

ret = StateMachine.LoadModel("configs/models/sample_ble_model.json") # Load State Machine model
if not ret:
    print("Error loading state machine model")
    exit(1)
    
print(f'Total States Loaded: {StateMachine.TotalStates()}')
print(f'Total Transitions Loaded: {StateMachine.TotalTransitions()}')


for dir, pkt in interesting_pkts:
    print('---------------------------------------------------------')
    pkt = bytearray(raw(pkt))  # Convert to raw and then to bytearray
    print(f'{Fore.MAGENTA}1) BEFORE Transition:')
    print(f'{Fore.YELLOW}Previous State: {StateMachine.GetPreviousStateName()}')
    print(f'{Fore.CYAN}Current State: {StateMachine.GetCurrentStateName()}')
    next_states = StateMachine.GetNextStateNames()
    if len(next_states):
        print(f'Next Expected States:')
        for state in next_states:
            print(f'  {state}')
    # 1) Prepare State Mapper
    StateMachine.PrepareStateMapper(wd)
    # 2) Set packet direction (WD_DIR_TX or WD_DIR_RX) and decode packet
    wd_set_packet_direction(wd, dir)
    wd_packet_dissect(wd, pkt, len(pkt))
    # 3) Run State Mapper
    transition_ok = StateMachine.RunStateMapper(wd, dir == WD_DIR_TX) # 2nd argument force transition to TX state, so we just need to validate RX
    # 4) Validate transition
    dir_str = "TX" if dir == WD_DIR_TX else "RX"
    print(f'\nReceived {dir_str}: {wd_packet_summary(wd)}\n')
    print(f'{Fore.MAGENTA}2) AFTER Transition ({dir_str}):')
    print(f'{Fore.YELLOW}Previous State: {StateMachine.GetPreviousStateName()}')
    print(f'{Fore.CYAN}Current State: {StateMachine.GetCurrentStateName()}')
    if dir == WD_DIR_RX:
        color = Fore.GREEN if transition_ok else Fore.RED
        print(f'{color}RX Transition Valid? {transition_ok}')

