#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "pico/stdlib.h"
#include "pico/usb_device.h"
#include "configuration_manager.h"
#include "configuration_types.h"

static const default_configuration default_config = {
    .filters = {
        .filter = { FILTER_CONFIGURATION, sizeof(default_config.filters) },
        .f1 = {PEAKING,     38,   -19,  0.9},
        .f2 = {LOWSHELF,    2900,   2,  0.7},
        .f3 = {PEAKING,     430,    3,  3.5},
        .f4 = {HIGHSHELF,   8400,   2,  0.7},
        .f5 = {PEAKING,     4800,   3,    5}
    }
};

static uint8_t config_buffer[2][512];
static uint8_t active_index = 0;
static bool config_dirty = false;
static uint16_t write_offset = 0;
static uint16_t read_offset = 0;
static uint16_t config_length = 0;

bool validate_filter_configuration(filter_configuration_tlv *filters)
{
    if (filters->header.type != FILTER_CONFIGURATION)
    {
        printf("Error! Not a filter TLV (%x)..\n", filters->header.type);
        return false;
    }
    uint8_t *ptr = (uint8_t *)filters->header.value;
    const uint8_t *end = (uint8_t *)filters + filters->header.length;
    while ((ptr + 4) < end)
    {
        uint32_t type = *(uint32_t *)ptr;
        uint16_t remaining = (uint16_t)(end - ptr);
        printf("Found Filter Type %d (%p rem: %d)..\n", type, ptr, remaining);
        switch (type)
        {
        case LOWPASS:
        case HIGHPASS:
        case BANDPASSSKIRT:
        case BANDPASSPEAK:
        case NOTCH:
        case ALLPASS:
        {
            if (remaining < sizeof(filter2))
            {
                printf("Error! Not enough data left for filter2 (%d)..\n", remaining);
                return false;
            }
            filter2 *args = (filter2 *)ptr;
            ptr += sizeof(filter2);
            break;
        }
        case PEAKING:
        case LOWSHELF:
        case HIGHSHELF:
        {
            if (remaining < sizeof(filter3))
            {
                printf("Error! Not enough data left for filter3 (%d)..\n", remaining);
                return false;
            }
            filter3 *args = (filter3 *)ptr;
            printf("Args: %0.2f %0.2f, %0.2f\n", args->f0, args->dBgain, args->Q);
            ptr += sizeof(filter3);
            break;
        }
        default:
            printf("Unknown filter type\n");
            return false;
        }
    }
    if (ptr != end)
    {
        printf("Error! Did not consume the whole TLV (%p != %p)..\n", ptr, end);
        return false;
    }
    printf("Config looks good..\n");
    return true;
}

void load_config()
{
    // Copy data from flash
    uint8_t index = active_index ? 0 : 1;
    // Load data
    active_index = index;
    config_dirty = true;
}

void save_config()
{
    // Write data to flash
}

void config_in_packet(struct usb_endpoint *ep) {
    assert(ep->current_transfer);
    struct usb_buffer *buffer = usb_current_in_packet_buffer(ep);
    assert(buffer->data_max >= 3);
    buffer->data_len = 0;

    printf("Config In Packet: Buffer Len: %d, Max: %d\n", buffer->data_len, buffer->data_max);
    memcpy(buffer->data, "Config In Packet", 17);
    buffer->data_len = 64;

    usb_grow_transfer(ep->current_transfer, 1);
    usb_packet_done(ep);
}

void config_out_packet(struct usb_endpoint *ep) {
    struct usb_buffer *usb_buffer = usb_current_out_packet_buffer(ep);

    printf("Config Out Packet >>%s<< %d %d %d %d\n", usb_buffer->data, usb_buffer->data_len, ep->current_transfer->completed, ep->current_transfer->remaining_packets_to_handle, ep->current_transfer->remaining_packets_to_submit);

    usb_grow_transfer(ep->current_transfer, 1);
    usb_packet_done(ep);
}