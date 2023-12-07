

#include "btstack_config.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "hci_cmd.h"
#include "btstack_run_loop.h"

#include "btstack.h"

#define NUM_SERVICES 20

int conf_iocap = SSP_IO_CAPABILITY_NO_INPUT_NO_OUTPUT;
int conf_auth_req = SSP_IO_AUTHREQ_MITM_PROTECTION_NOT_REQUIRED_GENERAL_BONDING;
int conf_enable_bounding = 1;
int conf_discoverable = 0;
char conf_pin[5] = {'0', '0', '0', '0', '\x00'};

int flag_bounding_start = 0;
int flag_bounding_done = 0;
int flag_rfcomm_connection_start = 0;
int flag_rfcomm_connection_done = 0;

int var_rfcomm_opened_channels = 0;
int var_rfcomm_closed_channels = 0;
uint16_t var_rfcom_cids[20];

static btstack_timer_source_t timer_rfcomm_query;
static btstack_timer_source_t timer_rfcomm_connection;
static btstack_timer_source_t timer_ssp_pairing;
static bd_addr_t remote_addr;

static struct
{
    uint8_t channel_nr;
    char service_name[SDP_SERVICE_NAME_LEN + 1];
} services[NUM_SERVICES];

static uint8_t service_index = 0;
static uint8_t current_service_index = 0;

static btstack_packet_callback_registration_t hci_event_callback_registration;

static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

static btstack_context_callback_registration_t handle_sdp_client_query_request;

static void handle_start_sdp_client_query(void *context)
{
    UNUSED(context);
    printf("Starting RFCOMM Query\n");
    sdp_client_query_rfcomm_channel_and_name_for_uuid(&packet_handler, remote_addr, BLUETOOTH_ATTRIBUTE_PUBLIC_BROWSE_ROOT);
}

static void store_found_service(const char *name, uint8_t port)
{
    printf("APP: Service name: '%s', RFCOMM port %u\n", name, port);
    if (service_index < NUM_SERVICES)
    {
        services[service_index].channel_nr = port;
        strncpy(services[service_index].service_name, (char *)name, SDP_SERVICE_NAME_LEN);
        services[service_index].service_name[SDP_SERVICE_NAME_LEN] = 0;
        service_index++;
    }
    else
    {
        printf("APP: list full - ignore\n");
        return;
    }
}

static void report_found_services(void)
{
    printf("\n *** Client query response done. ");
    if (service_index == 0)
    {
        printf("No service found.\n\n");
    }
    else
    {
        printf("Found following %d services:\n", service_index);
    }
    int i;
    for (i = 0; i < service_index; i++)
    {
        printf("     Service name %s, RFCOMM port %u\n", services[i].service_name, services[i].channel_nr);
    }
    printf(" ***\n\n");
}

static void reset_vars()
{
    flag_bounding_start = 0;
    flag_bounding_done = 0;
    flag_rfcomm_connection_start = 0;
    flag_rfcomm_connection_done = 0;
    var_rfcomm_closed_channels = 0;
    var_rfcomm_opened_channels = 0;
    service_index = 0;
}

static void start_ssp_pairing()
{
    static int started = 0;
    if (!started)
    {
        started = 1;
        // set one-shot timer
        timer_ssp_pairing.process = &start_ssp_pairing;
        btstack_run_loop_set_timer(&timer_ssp_pairing, 500);
        btstack_run_loop_add_timer(&timer_ssp_pairing);
    }
    else
    {
        started = 0;
        printf("Starting Bounding with IOCap = %02X, AuthReq=%02X\n", conf_iocap, conf_auth_req);
        gap_dedicated_bonding(remote_addr, 0);
    }
}

static void start_rfcomm_query()
{
    static int started = 0;
    static int created = 0;
    if (!started)
    {
        started = 1;
        // set one-shot timer
        timer_rfcomm_query.process = &start_rfcomm_query;
        btstack_run_loop_set_timer(&timer_rfcomm_query, 1000);
        btstack_run_loop_add_timer(&timer_rfcomm_query);
    }
    else
    {
        started = 0;
        handle_sdp_client_query_request.callback = &handle_start_sdp_client_query;
        (void)sdp_client_register_query_callback(&handle_sdp_client_query_request);
    }
}

static void start_rfcomm_connection()
{
    static int started = 0;
    if (!started)
    {
        started = 1;
        // set one-shot timer
        timer_rfcomm_connection.process = &start_rfcomm_connection;
        btstack_run_loop_set_timer(&timer_rfcomm_connection, 500);
        btstack_run_loop_add_timer(&timer_rfcomm_connection);
    }
    else
    {
        started = 0;
        if (service_index == 0)
        {
            // Try to force connection to devices without rfcomm service
            printf("Fallback, connecting to %s, RFCOMM port %u\n", services[0].service_name, services[0].channel_nr);
            rfcomm_create_channel(&packet_handler, remote_addr, services[0].channel_nr, &var_rfcom_cids[0]);
            return;
        }
        while (var_rfcomm_opened_channels < service_index)
        {
            int j = var_rfcomm_opened_channels;
            printf("Connecting to %s, RFCOMM port %u\n", services[j].service_name, services[j].channel_nr);
            int res = rfcomm_create_channel(&packet_handler, remote_addr, services[j].channel_nr, &var_rfcom_cids[j]);

            if (!res) // Create channel ok
                return;
            else
                printf("Failed with status=%d\n", res);

            var_rfcomm_opened_channels++;
        }
    }
}

static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    UNUSED(channel);
    UNUSED(size);

    // printf("type:%d\n", packet_type);

    // HCI Events
    if (packet_type == HCI_EVENT_PACKET)
    {
        switch (hci_event_packet_get_type(packet))
        {

        case BTSTACK_EVENT_STATE:
            // BTstack activated, get started
            if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING)
            {
                // Start SDP Query
                start_rfcomm_query();
            }
            break;
        case HCI_EVENT_DISCONNECTION_COMPLETE:
            puts("HCI Disconnection received");
            printf("flag_bounding_start=%d,flag_rfcomm_connection_start=%d,flag_rfcomm_connection_done=%d\n", flag_bounding_start, flag_rfcomm_connection_start, flag_rfcomm_connection_done);
            if (flag_rfcomm_connection_done)
            {
                flag_rfcomm_connection_done = 0;
                reset_vars();
                start_rfcomm_query();
            }
            else if (flag_bounding_start)
            {
                flag_bounding_start = 0;
                start_ssp_pairing();
            }
            else if (flag_rfcomm_connection_start)
            {
                flag_rfcomm_connection_start = 0;
                start_rfcomm_connection();
            }
            else if (var_rfcomm_closed_channels < service_index)
            {
                start_rfcomm_connection();
            }

            break;
        case HCI_EVENT_PIN_CODE_REQUEST:
            // inform about pin code request
            printf("Pin code request - using %s\n", remote_addr);
            hci_event_pin_code_request_get_bd_addr(packet, remote_addr);
            // baseband address, pin length, PIN: c-string
            hci_send_cmd(&hci_pin_code_request_reply, &remote_addr, 4, conf_pin);
            break;
        case HCI_EVENT_SIMPLE_PAIRING_COMPLETE:
        {
            flag_bounding_done = 1;
            flag_rfcomm_connection_start = 1;
            uint8_t res = hci_event_simple_pairing_complete_get_status(packet);
            printf("Secure Simple Pairing complete status: %d\n", res);
            // Disconnection will be triggered automatically, altough some slaves may continue.
            if (res != 0)
            {
                reset_vars();
                flag_rfcomm_connection_done = 1;
                hci_disconnect_all();
            }
            break;
        }

        case RFCOMM_EVENT_CHANNEL_OPENED:

            // data: event(8), len(8), status (8), address (48), server channel(8), rfcomm_cid(16), max frame size(16)
            if (rfcomm_event_channel_opened_get_status(packet))
            {
                printf("RFCOMM channel open failed, status %u\n", rfcomm_event_channel_opened_get_status(packet));
            }
            else
            {
                int rfcomm_channel_id = rfcomm_event_channel_opened_get_rfcomm_cid(packet);
                int mtu = rfcomm_event_channel_opened_get_max_frame_size(packet);
                printf("RFCOMM channel open succeeded. New RFCOMM Channel ID %u, max frame size %u\n", rfcomm_channel_id, mtu);
            }
            var_rfcomm_opened_channels++;
            // Open new channel, otherwise close all, service_index 0 will force connection
            if ((var_rfcomm_opened_channels < service_index))
            {
                // Open all RFCOMM channels
                start_rfcomm_connection();
            }
            else
            {
                // Disconnect from all RFCOMM channels
                for (int i = 0; i < service_index; i++)
                {
                    rfcomm_disconnect(var_rfcom_cids[i]);
                }
                if (service_index == 0)
                {
                    flag_rfcomm_connection_done = 1;
                    hci_disconnect_all();
                }
            }

            break;
        case RFCOMM_EVENT_CHANNEL_CLOSED:
            var_rfcomm_closed_channels++;
            printf("RFCOMM channel closed, idx=%d/%d\n", var_rfcomm_closed_channels, service_index);

            if (var_rfcomm_closed_channels >= service_index)
            {
                printf("All done!\n");
                flag_rfcomm_connection_done = 1;
                hci_disconnect_all();
            }

            break;

        default:
            switch (packet[0])
            {
            case SDP_EVENT_QUERY_RFCOMM_SERVICE:
                store_found_service(sdp_event_query_rfcomm_service_get_name(packet),
                                    sdp_event_query_rfcomm_service_get_rfcomm_channel(packet));
                break;
            case SDP_EVENT_QUERY_COMPLETE:
                if (sdp_event_query_complete_get_status(packet))
                {
                    printf("SDP query failed 0x%02x, retrying...\n", sdp_event_query_complete_get_status(packet));
                    sdp_client_query_rfcomm_channel_and_name_for_uuid(&packet_handler, remote_addr, BLUETOOTH_ATTRIBUTE_PUBLIC_BROWSE_ROOT);
                }
                else
                {
                    printf("SDP query done.\n");
                    report_found_services();
                    if (!flag_bounding_done)
                    {
                        flag_bounding_start = 1;
                        hci_disconnect_all();
                    }
                }

                break;
            }
        }
    }
}

#ifdef HAVE_POSIX_FILE_IO
static void usage(const char *name)
{
    printf("\nUsage: %s -a|--address aa:bb:cc:dd:ee:ff\n", name);
    printf("Use argument -a to connect to a specific device and dump the result of SDP query for L2CAP services.\n\n");
}
#endif

void signal_handler(int signal)
{
    exit(0);
}

int btstack_main(int argc, const char *argv[]);
int btstack_main(int argc, const char *argv[])
{
    (void)argc;
    (void)argv;

    int remote_addr_found = 0;
    for (size_t i = 0; i < argc; i++)
    {
        if (argc > i + 1)
        {
            if (!strcmp(argv[i], "-a") || !strcmp(argv[i], "--address"))
            {
                remote_addr_found = sscanf_bd_addr(argv[i + 1], remote_addr);
                printf("address=%s\n", argv[i + 1]);
            }
            else if (!strcmp(argv[i], "--iocap"))
            {
                conf_iocap = atoi(argv[i + 1]);
                printf("iocap=%d\n", conf_iocap);
            }
            else if (!strcmp(argv[i], "--authreq"))
            {
                conf_auth_req = atoi(argv[i + 1]);
                printf("authreq=%d\n", conf_auth_req);
            }
            else if (!strcmp(argv[i], "--bounding"))
            {
                conf_enable_bounding = atoi(argv[i + 1]);
                printf("bouding=%d\n", conf_enable_bounding);
            }
            else if (!strcmp(argv[i], "--discoverable"))
            {
                conf_discoverable = atoi(argv[i + 1]);
                printf("discoverable=%d\n", conf_discoverable);
            }
        }
    }

    if (!remote_addr_found)
    {
        usage(argv[0]);
        exit(1);
    }

    signal(SIGTERM, signal_handler);

    // init L2CAP
    l2cap_init();
    rfcomm_init();

    // register for HCI events
    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    gap_delete_all_link_keys();
    gap_ssp_set_io_capability(conf_iocap);
    gap_ssp_set_authentication_requirement(conf_auth_req);
    gap_set_bondable_mode(conf_enable_bounding);
    gap_discoverable_control(conf_discoverable);
    gap_set_local_name("BT Fuzzer 00:00:00:00:00:00");
    hci_set_inquiry_mode(INQUIRY_MODE_STANDARD);
    // hci_set_master_slave_policy(HCI_ROLE_MASTER);
    // hci_set_master_slave_policy(HCI_ROLE_SLAVE);
    // turn on!
    hci_power_control(HCI_POWER_ON);

    return 0;
}
