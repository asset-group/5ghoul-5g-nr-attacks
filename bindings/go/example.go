package main

import (
	"./wdissector" // use "wdissector" to use system package instead of local one
	"fmt"
	"time"
)

func main() {

	fmt.Println("------------------------------")
	fmt.Println("WDissector GO Example")
	fmt.Println("------------------------------")

	// Set library path to environment variable WDISSECTOR_PATH
	wdissector.Wdissector_link_library()
	// wdissector.Wdissector_link_library("/path/to/wdissector/")

	// Init library with protocol "btacl"
	wdissector.Wdissector_init("encap:BLUETOOTH_HCI_H4") // Format for general bluetooth pcap
	fmt.Println("Info: " + wdissector.Wdissector_version_info())
	fmt.Println("Loaded Profile: " + wdissector.Wdissector_profile_info())
	// Example packet
	var bt_hci_packet = []byte{0x1, 0x8, 0x20, 0x20, 0x1d, 0x2, 0x1, 0x6, 0x3,
		0x3, 0x27, 0x18, 0x15, 0x16, 0x27, 0x18, 0xde,
		0x22, 0xf0, 0xa6, 0x3d, 0x80, 0xd9, 0x74, 0x87,
		0x48, 0xb3, 0x9f, 0x54, 0x36, 0x5a, 0xc3, 0x0,
		0x0, 0x0, 0x0}

	fmt.Println("\n------------- Packet Summary --------------")
	// Need to pass packet byte pointer in &[i] format
	// Packet size needs to be unsigned
	wdissector.Packet_set_direction(0) // 1 - RX, 0 - TX
	wdissector.Packet_dissect(&bt_hci_packet[0], uint(len(bt_hci_packet)))
	// Print packet summary
	fmt.Println(wdissector.Packet_summary())
	fmt.Println("Layers: " + wdissector.Packet_dissectors())
	fmt.Println("Description: \n" + wdissector.Packet_description())
	fmt.Println("-------------------------------------------")

	fmt.Println("\n---------- API DEMO (by string) -----------")
	wdissector.Packet_dissect(&bt_hci_packet[0], uint(len(bt_hci_packet)))

	var field_name = "btcommon.eir_ad.entry.uuid_16"
	if wdissector.Packet_get_field_exists(field_name) > 0 {
		var full_name = wdissector.Packet_get_field_name(field_name)
		var offset = wdissector.Packet_get_field_offset(field_name)
		var size_bytes = wdissector.Packet_get_field_size(field_name)
		var value = wdissector.Packet_get_field_uint32(field_name)

		var field_type = wdissector.Packet_get_field_type_name(field_name)
		var encoding_name = wdissector.Packet_get_field_encoding_name(field_name)

		var bitmask = wdissector.Packet_get_field_bitmask(field_name)
		var bitmask_offset = wdissector.Packet_read_field_bitmask_offset(bitmask)
		var bitmask_size = wdissector.Packet_read_field_size_bits(bitmask)

		fmt.Printf("[ Field: %s ]\n", field_name)
		fmt.Printf("Name: %s\n", full_name)
		fmt.Printf("Offset: %d\n", offset)
		fmt.Printf("Length: %d\n", size_bytes)
		fmt.Printf("Value: %d\n", value)

		fmt.Printf("Type: %s\n", field_type)
		fmt.Printf("Encoding: %s\n", encoding_name)

		fmt.Printf("Bitmask: 0x%08x\n", bitmask)
		fmt.Printf("Bitmask Offset: %d\n", bitmask_offset)
		fmt.Printf("Bitmask Length: %d\n", bitmask_size)

	} else {
		fmt.Printf("Field %s not found in packet\n", field_name)
	}

	fmt.Println("\n----- API DEMO (by field info FASTER) ------")
	var field_name2 = "btcommon.eir_ad.entry.flags.bredr_not_supported"
	var field_info = wdissector.Packet_register_set_field_hfinfo(field_name2)
	wdissector.Packet_dissect(&bt_hci_packet[0], uint(len(bt_hci_packet)))

	if wdissector.Packet_read_field_exists_hfinfo(field_info) > 0 {
		var field = wdissector.Packet_read_field_hfinfo(field_info)

		var full_name = wdissector.Packet_read_field_name(field)
		var offset = wdissector.Packet_read_field_offset(field)
		var value = wdissector.Packet_read_field_uint32(field)
		var size_bytes = wdissector.Packet_read_field_size(field)

		var field_type = wdissector.Packet_read_field_type_name(field)
		var encoding_name = wdissector.Packet_read_field_encoding_name(field)

		var bitmask = wdissector.Packet_read_field_bitmask(field)
		var bitmask_offset = wdissector.Packet_read_field_bitmask_offset(bitmask)
		var bitmask_size = wdissector.Packet_read_field_size_bits(bitmask)

		fmt.Printf("[ Field: %s ]\n", field_name2)
		fmt.Printf("Name: %s\n", full_name)
		fmt.Printf("Offset: %d\n", offset)
		fmt.Printf("Length: %d\n", size_bytes)
		fmt.Printf("Value: %d\n", value)

		fmt.Printf("Type: %s\n", field_type)
		fmt.Printf("Encoding: %s\n", encoding_name)

		fmt.Printf("Bitmask: 0x%08x\n", bitmask)
		fmt.Printf("Bitmask Offset: %d\n", bitmask_offset)
		fmt.Printf("Bitmask Length: %d\n", bitmask_size)

	} else {
		fmt.Printf("Field %s not found in packet\n", field_name2)
	}

	fmt.Println("\nDone. Enjoy!")

	time.Sleep(1 * time.Second)

}
