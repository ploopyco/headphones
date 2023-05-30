#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "pico/stdlib.h"
#include "pico/usb_device.h"
#include "configuration_manager.h"

static uint8_t config_buffer[2][512];
static uint8_t active_index = 0;
static bool config_dirty = false;
static uint16_t write_offset = 0;
static uint16_t read_offset = 0;
static uint16_t config_length = 0;

typedef struct _TLVHeader {
    uint16_t type;
    uint16_t length;
} TLVHeader;

typedef struct _ConfigHeaderTLV {
    struct _TLVHeader header;
    uint32_t config_magic;
    uint16_t config_version;
    uint16_t reserved;
} ConfigHeaderTLV;

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