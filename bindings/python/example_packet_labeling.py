#!/usr/bin/env python3

from binascii import hexlify, unhexlify
from wdissector import WDPacketLabelGenerator, wd_pkt_label, WD_DIR_TX, WD_DIR_RX
from scapy.utils import rdpcap
from scapy.packet import raw
from colorama import Fore, Back, Style
import colorama

pkt_decoding_offset = 0

def label_packets(packets_to_label, pkt_decoding_offset = 0):
    for pkt_dir, pkt in packets_to_label:
        print('---------------------------------------------------------')
        # Convert to raw and then to bytearray
        pkt = bytearray(raw(pkt))[pkt_decoding_offset:]
  
        # Try generating a label for this packet and collect the matched field and value info
        PktLabel = PktLabelGen.LabelPacket(pkt_dir, pkt, len(pkt))

        if PktLabel.label_status is True:
            print(f'{Fore.GREEN}Label Status: {PktLabel.label_status}')
            print(f'{Fore.CYAN}Processing Time: {PktLabel.label_timing_ns / 1000}us')
            print(f'Packet Summary: {PktLabel.pkt_summary}')
            print(f'Packet Label: {PktLabel.pkt_label}')
            print(f'{Fore.YELLOW}Packet State Field Name: {PktLabel.pkt_field_name}')
            print(f'{Fore.CYAN}Packet State Field Value: {PktLabel.pkt_field_value}')
            # Create wireshark display filter that matches this packet according to the mapping rules in the config. file
            print(f'{Fore.MAGENTA}Packet Filter (Wireshark): "{Fore.YELLOW}{PktLabel.pkt_field_name}{Fore.RESET} == {Fore.CYAN}{PktLabel.pkt_field_value}"')
        else:
            print(f'{Fore.RED}Label Status: {PktLabel.label_status}')
            return None

colorama.init(autoreset=True)


# ------------------ Packet Label Generator Initialization ------------------
print('\n\n--------------------- Packet Label Generator ---------------------')
PktLabelGen = WDPacketLabelGenerator()
PktLabel = wd_pkt_label()

# ------------------ Example with BLE ------------------
print(f'\n{Fore.BLACK}{Back.WHITE}<============> 1) Example with BLE Packets <============>')

pkts = rdpcap("captures/capture_dialog_DA14680_truncated_l2cap_crash.pcap")

interesting_pkts = []
interesting_pkts.append((WD_DIR_TX, pkts[58]))
interesting_pkts.append((WD_DIR_RX, pkts[60]))
interesting_pkts.append((WD_DIR_TX, pkts[64]))
interesting_pkts.append((WD_DIR_RX, pkts[66]))

ret = PktLabelGen.init("configs/ble_config.json", True) # Load State Mapper configuration
if not ret:
    print("Error initializing packet label generator")
    exit(1)

label_packets(interesting_pkts)

# ------------------ Example with 5G ------------------
print(f'\n{Fore.BLACK}{Back.WHITE}<============> 2) Example with 5G NR Packets <============>')
pkts = rdpcap("captures/capture_5gnr_mtk_crash.pcapng")

interesting_pkts = []
interesting_pkts.append((WD_DIR_TX, pkts[9])) # Packet number 10 in wireshark capture
interesting_pkts.append((WD_DIR_TX, pkts[13])) # Packet number 14 in wireshark capture

ret = PktLabelGen.init("configs/5gnr_gnb_config.json", True) # Load State Mapper configuration
if not ret:
    print("Error initializing packet label generator")
    exit(1)

# ------------------ Example with BT Classic ------------------
print(f'\n{Fore.BLACK}{Back.WHITE}<============> 3) Example with Bluetooth Classic <============>')
pkts = rdpcap("captures/capture_bluetooth_iocap_dup.pcapng")

interesting_pkts = []
interesting_pkts.append((WD_DIR_TX, pkts[8])) # Packet number 10 in wireshark capture
interesting_pkts.append((WD_DIR_TX, pkts[9])) # Packet number 14 in wireshark capture

ret = PktLabelGen.init("configs/bt_config.json", True) # Load State Mapper configuration
if not ret:
    print("Error initializing packet label generator")
    exit(1)

label_packets(interesting_pkts, 4)
