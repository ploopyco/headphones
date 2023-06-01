/**
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "pico/stdlib.h"
#include "pico/usb_device.h"
#include "configuration_manager.h"
#include "configuration_types.h"

// TODO: Duplicated from os_descriptors.h
#define U16_HIGH(_u16)          ((uint8_t) (((_u16) >> 8) & 0x00ff))
#define U16_LOW(_u16)           ((uint8_t) ((_u16)       & 0x00ff))

#define U16_TO_U8S_LE(_u16)     U16_LOW(_u16), U16_HIGH(_u16)

/**
 * We have multiple copies of the device configuration. This is the factory
 * default configuration, it is static data in the firmware.
 * We also potentially have a user configuration stored at the end of flash
 * memory. And an in RAM working configuration.
 *
 * The idea is that when the device boots, it tries to use the user config
 * from the end of flash. If that is not present, or is invalid, we use this
 * default config instead.
 *
 * If the user sends an updated configuration over the USB port, it is stored
 * in RAM as a working configuration, and is used (until we lose power). If
 * the user issues a save command the working configuration is written to flash
 * and becomes the new user configuration. 
 */
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

/**
 * TODO: For now, assume we always get a complete configuration but maybe we
 * should handle merging configurations where, for example, only a new
 * filter_configuration_tlv was received.
 */
static uint8_t working_configuration[256];
static uint8_t result_buffer[256] = { U16_TO_U8S_LE(NOK), U16_TO_U8S_LE(4) };

static bool config_dirty = false;
static uint16_t write_offset = 0;
static uint16_t read_offset = 0;

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
            printf("Args: F0: %0.2f, Q: %0.2f\n", args->f0, args->Q);
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
            printf("Args: F0: %0.2f, dbGain: %0.2f, Q: %0.2f\n", args->f0, args->dBgain, args->Q);
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

bool validate_configuration(tlv_header *config)
{
    if (config->type != SET_CONFIGURATION) {
        printf("Unexpcected Config type: %d\n", config->type);
        return false;
    }
    uint8_t *ptr = (uint8_t *)config->value;
    const uint8_t *end = (uint8_t *)config + config->length;
    while (ptr < end) {
        tlv_header* tlv = (tlv_header*) ptr;
        printf("Found TLV type: %d\n", tlv->type);
        if (tlv->type == FILTER_CONFIGURATION)
        {
            if (!validate_filter_configuration((filter_configuration_tlv*) tlv)) {
                return false;
            }
        }

        ptr += tlv->length;
    }
}

void load_config()
{
    // Try to load data from flash
    // If that is no good, use the default config
}

void save_config()
{
    // Write data to flash
}

// This callback is called when the client sends a message to the device.
// We implement a simple messaging protocol. The client sends us a message that
// we consume here. All messages are constructed of TLV's (Type Length Value).
// In some cases the Value may be a set of TLV's. However, each message has an
// owning TLV, and its length determines the length of the transfer.
// Once we have consumed the whole message, we validate it and populate the result
// buffer with a TLV which we expect the client to read next.
void config_out_packet(struct usb_endpoint *ep) {
    struct usb_buffer *buffer = usb_current_out_packet_buffer(ep);
    printf("config_out_packet %d\n", buffer->data_len);

    memcpy(&working_configuration[write_offset], buffer->data, buffer->data_len);
    write_offset += buffer->data_len;

    const uint16_t transfer_length = ((tlv_header*) working_configuration)->length;
    printf("config_length %d %d\n", transfer_length, write_offset);
    if (transfer_length >= write_offset) {
        // Command complete, fill the result buffer
        tlv_header* result = ((tlv_header*) result_buffer);
        write_offset = 0;

        if (validate_configuration((tlv_header*) working_configuration)) {
            result->type = OK;
            result->length = 4;
        }
        else {
            result->type = NOK;
            result->length = 4;
        }
    }

    usb_grow_transfer(ep->current_transfer, 1);
    usb_packet_done(ep);
}

// This callback is called when the client attempts to read data from the device.
// The client should have previously written a command which will have populated the
// result_buffer. The client should attempt to read 4 bytes (the Type and Length)
// then attempt to read the rest of the data once the length is known.
void config_in_packet(struct usb_endpoint *ep) {
    assert(ep->current_transfer);
    struct usb_buffer *buffer = usb_current_in_packet_buffer(ep);
    printf("config_in_packet %d\n", buffer->data_len);
    assert(buffer->data_max >= 3);

    const uint16_t transfer_length = ((tlv_header*) result_buffer)->length;
    const uint16_t packet_length = MIN(buffer->data_max, transfer_length - read_offset);
    memcpy(buffer->data, &result_buffer[read_offset], packet_length);
    buffer->data_len = packet_length;

    if (transfer_length >= read_offset) {
        // Done
        read_offset = 0;

        // If the client reads again, return an error
        tlv_header* result = ((tlv_header*) result_buffer);
        result->type = NOK;
        result->length = 4;
    }

    usb_grow_transfer(ep->current_transfer, 1);
    usb_packet_done(ep);
}