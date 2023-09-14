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
#ifndef __CONFIGURATION_TYPES_H__
#define __CONFIGURATION_TYPES_H__
#include <stdint.h>

#define FLASH_MAGIC 0x2E8AFEDD
#define CONFIG_VERSION 4
#define MINIMUM_CONFIG_VERSION 4

enum structure_types {
    // Commands/Responses, these are container TLVs. The Value will be a set of TLV structures.
    OK = 0,                     // Standard response when a command was successful
    NOK,                        // Standard error response
    FLASH_HEADER,               // A special container for the config stored in flash. Hopefully there is some useful
                                // metadata in here to allow us to migrate an old config to a new version.
    GET_VERSION,                // Returns the current config version, and the minimum supported version so clients
                                // can decide if they can talk to us or not.
    SET_CONFIGURATION,          // Updates the active configuration with the supplied TLVs
    GET_ACTIVE_CONFIGURATION,   // Retrieves the current active configuration TLVs from RAM
    GET_STORED_CONFIGURATION,   // Retrieves the current stored configuration TLVs from Flash
    SAVE_CONFIGURATION,         // Writes the active configuration to Flash
    FACTORY_RESET,              // Invalidates the flash memory

    // Configuration structures, these are returned in the body of a command/response
    PREPROCESSING_CONFIGURATION = 0x200,
    FILTER_CONFIGURATION,
    PCM3060_CONFIGURATION,

    // Status structures, these are returned in the body of a command/response but they are
    // not persisted as part of the configuration
    VERSION_STATUS = 0x400,
};

typedef struct __attribute__((__packed__)) _tlv_header {
    uint16_t type;
    uint16_t length;
    const uint8_t value[0]; // Doesn't take up any space, just a convenient way of accessing the value
} tlv_header;

typedef struct __attribute__((__packed__)) _filter2 {
    uint8_t type;
    uint8_t reserved[3];
    float f0;
    float Q;
} filter2;

typedef struct __attribute__((__packed__)) _filter3 {
    uint8_t type;
    uint8_t reserved[3];
    float f0;
    float db_gain;
    float Q;
} filter3;

// WARNING: We wont be able to support more than 8 of these filters
// due to the config structure size.
typedef struct __attribute__((__packed__)) _filter6 {
    uint8_t type;
    uint8_t reserved[3];
    double a0;
    double a1;
    double a2;
    double b0;
    double b1;
    double b2;
} filter6;

enum filter_type {
    LOWPASS = 0,
    HIGHPASS,
    BANDPASSSKIRT,
    BANDPASSPEAK,
    NOTCH,
    ALLPASS,
    PEAKING,
    LOWSHELF,
    HIGHSHELF,
    CUSTOMIIR
};

typedef struct __attribute__((__packed__)) _flash_header_tlv {
    tlv_header header;
    uint32_t magic;
    uint32_t version;
    const uint8_t tlvs[0];
} flash_header_tlv;

/// @brief Holds values relating to processing surrounding the EQ calculation.
typedef struct __attribute__((__packed__)) _preprocessing_configuration_tlv {
    tlv_header header;
    /// @brief Gain applied to input signal before EQ chain. Use to avoid clipping due to overflow in the biquad filters of the EQ.
    float preamp;
    /// @brief Gain applied to the output of the EQ chain. Used to set output volume.
    float postEQGain;
    uint8_t reverse_stereo;
    uint8_t reserved[3];
} preprocessing_configuration_tlv;

typedef struct __attribute__((__packed__)) _filter_configuration_tlv {
    tlv_header header;
    const uint8_t filters[0];
} filter_configuration_tlv;

typedef struct __attribute__((__packed__)) _pcm3060_configuration_tlv {
    tlv_header header;
    const uint8_t oversampling;
    const uint8_t phase;
    const uint8_t rolloff;
    const uint8_t de_emphasis;
} pcm3060_configuration_tlv;


typedef struct __attribute__((__packed__)) _version_status_tlv {
    tlv_header header;
    uint16_t current_version;
    uint16_t minimum_supported_version;
    uint32_t reserved;
    const char version_strings[0];  // Firmware version\0Pico SDK version\0
} version_status_tlv;

typedef struct __attribute__((__packed__)) _default_configuration {
    tlv_header set_configuration;
    const struct __attribute__((__packed__)) {
        tlv_header filter;
        filter3 f1;
        filter3 f2;
        filter3 f3;
        filter3 f4;
        filter3 f5;
        filter3 f6;
        filter3 f7;
        filter3 f8;
        filter3 f9;
        filter3 f10;
        filter3 f11;
        filter3 f12;
        filter3 f13;
        filter3 f14;
        filter3 f15;
    } filters;
    preprocessing_configuration_tlv preprocessing;
} default_configuration;

#endif // __CONFIGURATION_TYPES_H__