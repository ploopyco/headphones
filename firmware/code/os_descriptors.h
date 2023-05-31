#ifndef __OS_DESCRIPTORS__
#define __OS_DESCRIPTORS__
#include <stdint.h>

#define TU_U16_HIGH(_u16)      ((uint8_t) (((_u16) >> 8) & 0x00ff))
#define TU_U16_LOW(_u16)       ((uint8_t) ((_u16)       & 0x00ff))
#define U16_TO_U8S_BE(_u16)    TU_U16_HIGH(_u16), TU_U16_LOW(_u16)
#define U16_TO_U8S_LE(_u16)    TU_U16_LOW(_u16), TU_U16_HIGH(_u16)
#define TU_U32_BYTE3(_u32)     ((uint8_t) ((((uint32_t) _u32) >> 24) & 0x000000ff)) // MSB
#define TU_U32_BYTE2(_u32)     ((uint8_t) ((((uint32_t) _u32) >> 16) & 0x000000ff))
#define TU_U32_BYTE1(_u32)     ((uint8_t) ((((uint32_t) _u32) >>  8) & 0x000000ff))
#define TU_U32_BYTE0(_u32)     ((uint8_t) (((uint32_t)  _u32)        & 0x000000ff)) // LSB

#define U32_TO_U8S_BE(_u32)    TU_U32_BYTE3(_u32), TU_U32_BYTE2(_u32), TU_U32_BYTE1(_u32), TU_U32_BYTE0(_u32)
#define U32_TO_U8S_LE(_u32)    TU_U32_BYTE0(_u32), TU_U32_BYTE1(_u32), TU_U32_BYTE2(_u32), TU_U32_BYTE3(_u32)

#define MS_OS_20_DESC_LEN  0xB2

typedef enum {
    MS_OS_20_SET_HEADER_DESCRIPTOR       = 0x00,
    MS_OS_20_SUBSET_HEADER_CONFIGURATION = 0x01,
    MS_OS_20_SUBSET_HEADER_FUNCTION      = 0x02,
    MS_OS_20_FEATURE_COMPATBLE_ID        = 0x03,
    MS_OS_20_FEATURE_REG_PROPERTY        = 0x04,
    MS_OS_20_FEATURE_MIN_RESUME_TIME     = 0x05,
    MS_OS_20_FEATURE_MODEL_ID            = 0x06,
    MS_OS_20_FEATURE_CCGP_DEVICE         = 0x07,
    MS_OS_20_FEATURE_VENDOR_REVISION     = 0x08
} microsoft_os_20_type_t;

static __aligned(4) uint8_t ms_platform_capability_bos_descriptor[PICO_USBDEV_MAX_DESCRIPTOR_SIZE] = {
    // BOS Descriptor
    // length, descriptor type, total length, number of device caps
    0x5, 0xF, 0x21, 0x0, 0x1,
    // Platform device capability descriptor
    // length, descriptor type, device capability type, reserved
    0x1C, 0x10, 0x5, 0x00,
    // PlatformCapabilityUUID: D8DD60DF-4589-4CC7-9CD2-659D9E648A9F - Microsoft OS 2.0 descriptor capability
    0xDF, 0x60, 0xDD, 0xD8, 0x89, 0x45, 0xC7, 0x4C, 0x9C, 0xD2, 0x65, 0x9D, 0x9E, 0x64, 0x8A, 0x9F,
    // Windows version (8.1+)
    0x0, 0x0, 0x3, 0x6,
    // Length of desc_ms_os_20
    U16_TO_U8S_LE(MS_OS_20_DESC_LEN),
    // Vendor code, alt enum code
    0x1, 0x0
};

uint8_t desc_ms_os_20[PICO_USBDEV_MAX_DESCRIPTOR_SIZE] = {
    // Set header: length, type, windows version, total length
    U16_TO_U8S_LE(0x000A), U16_TO_U8S_LE(MS_OS_20_SET_HEADER_DESCRIPTOR), U32_TO_U8S_LE(0x06030000), U16_TO_U8S_LE(MS_OS_20_DESC_LEN),

    // Configuration subset header: length, type, configuration index, reserved, configuration total length
    U16_TO_U8S_LE(0x0008), U16_TO_U8S_LE(MS_OS_20_SUBSET_HEADER_CONFIGURATION), 0, 0, U16_TO_U8S_LE(MS_OS_20_DESC_LEN-0x0A),

    // Function Subset header: length, type, first interface, reserved, subset length
    U16_TO_U8S_LE(0x0008), U16_TO_U8S_LE(MS_OS_20_SUBSET_HEADER_FUNCTION), 2  /*interface*/, 0, U16_TO_U8S_LE(MS_OS_20_DESC_LEN-0x0A-0x08),

    // MS OS 2.0 Compatible ID descriptor: length, type, compatible ID, sub compatible ID
    U16_TO_U8S_LE(0x0014), U16_TO_U8S_LE(MS_OS_20_FEATURE_COMPATBLE_ID), 'W', 'I', 'N', 'U', 'S', 'B', 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // sub-compatible

    // MS OS 2.0 Registry property descriptor: length, type
    U16_TO_U8S_LE(MS_OS_20_DESC_LEN-0x0A-0x08-0x08-0x14), U16_TO_U8S_LE(MS_OS_20_FEATURE_REG_PROPERTY),
    U16_TO_U8S_LE(0x0007), U16_TO_U8S_LE(0x002A), // wPropertyDataType, wPropertyNameLength and PropertyName "DeviceInterfaceGUIDs\0" in UTF-16
    'D', 0x00, 'e', 0x00, 'v', 0x00, 'i', 0x00, 'c', 0x00, 'e', 0x00, 'I', 0x00, 'n', 0x00, 't', 0x00, 'e', 0x00,
    'r', 0x00, 'f', 0x00, 'a', 0x00, 'c', 0x00, 'e', 0x00, 'G', 0x00, 'U', 0x00, 'I', 0x00, 'D', 0x00, 's', 0x00, 0x00, 0x00,
    U16_TO_U8S_LE(0x0050), // wPropertyDataLength
    //bPropertyData: “{E8379B1D-6AA3-F426-2EAE-83D18090CA79}”.
    '{', 0x00, 'E', 0x00, '8', 0x00, '3', 0x00, '7', 0x00, '9', 0x00, 'B', 0x00, '1', 0x00, 'D', 0x00, '-', 0x00,
    '6', 0x00, 'A', 0x00, 'A', 0x00, '3', 0x00, '-', 0x00, 'F', 0x00, '4', 0x00, '2', 0x00, '6', 0x00, '-', 0x00,
    '2', 0x00, 'E', 0x00, 'A', 0x00, 'E', 0x00, '-', 0x00, '8', 0x00, '3', 0x00, 'D', 0x00, '1', 0x00, '8', 0x00,
    '0', 0x00, '9', 0x00, '0', 0x00, 'C', 0x00, 'A', 0x00, '7', 0x00, '9', 0x00, '}', 0x00, 0x00, 0x00, 0x00, 0x00
};

#endif //__OS_DESCRIPTORS__