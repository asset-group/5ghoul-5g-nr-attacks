#!/usr/bin/env bash

ln -s ../../bin -f -r bin
../../bin/wdmapper -c config_amqp.json -i capture_amqp.pcapng -o states_amqp.svg
../../bin/wdmapper -c config_ble_gatt.json -i capture_ble_gatt.pcapng -o states_ble_gatt.svg 
../../bin/wdmapper -c config_bt.json -i capture_bt_a2dp.pcapng -o states_bt_a2dp.svg 
../../bin/wdmapper -c config_coap.json -i capture_coap.pcapng -o states_coap.svg
../../bin/wdmapper -c config_4g.json -i capture_4g.pcapng -o states_4g.svg
../../bin/wdmapper -c config_5g.json -i capture_5g_nsa_enb.pcapng  -o states_5g_nsa_merge1.svg
../../bin/wdmapper -c config_5g.json -i capture_5g_nsa_gnb.pcapng  -o states_5g_nsa_merge2.svg
../../bin/wdmapper -c config_5g.json -i capture_5g_nsa_enb.pcapng -i capture_5g_nsa_gnb.pcapng  -o states_5g_nsa.svg
../../bin/wdmapper -c config_wifi_ap.json -i capture_wifi_ap_eap_peap.pcapng -o states_wifi_ap_eap.svg
