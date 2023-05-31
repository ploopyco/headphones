#ifndef __CONFIGURATION_TYPES_H__
#define __CONFIGURATION_TYPES_H__
#include <stdint.h>

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
    uint32_t type;
    double f0;
    double Q;
} filter2;

typedef struct __attribute__((__packed__)) _filter3 {
    uint32_t type;
    double f0;
    double dBgain;
    double Q;
} filter3;

enum filter_type {
    LOWPASS = 0,
    HIGHPASS,
    BANDPASSSKIRT,
    BANDPASSPEAK,
    NOTCH,
    ALLPASS,
    PEAKING,
    LOWSHELF,
    HIGHSHELF
};

typedef struct __attribute__((__packed__)) _filter_configuration_tlv {
    tlv_header header;
    const uint8_t filters[];
} filter_configuration_tlv;

typedef struct __attribute__((__packed__)) _version_status_tlv {
    tlv_header header;
    uint16_t current_version;
    uint16_t minimum_supported_version;
} version_status_tlv;

typedef struct __attribute__((__packed__)) _default_configuration {
    const struct __attribute__((__packed__)) {
        tlv_header filter;
        filter3 f1;
        filter3 f2;
        filter3 f3;
        filter3 f4;
        filter3 f5;
    } filters;
} default_configuration;

#endif // __CONFIGURATION_TYPES_H__