/**
 * Copyright 2022 Colin Lam, Ploopy Corporation
 *
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
 *
 * SPECIAL THANKS TO:
 * @kilograham (github.com/kilograham)
 * for his exceptional work on Pico Playground's usb-sound-card, on which
 * a large portion of this work is based.
 */

#ifndef RUN_H
#define RUN_H

#include "pico/usb_device.h"
#include "AudioClassCommon.h"

#include "ringbuf.h"
#include "i2s.h"

/*****************************************************************************
 * USB-related definitions begin here.
 ****************************************************************************/

// todo fix these

// actually windows doesn't seem to like this in the middle, so set top range to 0db
#define CENTER_VOLUME_INDEX 91

#define ENCODE_DB(x) ((uint16_t)(int16_t)((x)*256))

#define MIN_VOLUME ENCODE_DB(-100)
#define DEFAULT_VOLUME ENCODE_DB(0)
#define MAX_VOLUME ENCODE_DB(0)
#define VOLUME_RESOLUTION ENCODE_DB(0.5f)

typedef struct _audio_device_config {
    struct usb_configuration_descriptor descriptor;
    struct usb_interface_descriptor ac_interface;
    struct __packed {
        USB_Audio_StdDescriptor_Interface_AC_t core;
        USB_Audio_StdDescriptor_InputTerminal_t input_terminal;
        USB_Audio_StdDescriptor_FeatureUnit_t feature_unit;
        USB_Audio_StdDescriptor_OutputTerminal_t output_terminal;
    } ac_audio;
    struct usb_interface_descriptor as_zero_interface;
    struct usb_interface_descriptor as_op_interface;
    struct __packed {
        USB_Audio_StdDescriptor_Interface_AS_t streaming;
        struct __packed {
            USB_Audio_StdDescriptor_Format_t core;
            USB_Audio_SampleFreq_t freqs[2];
        } format;
    } as_audio;
    struct __packed {
        struct usb_endpoint_descriptor_long core;
        USB_Audio_StdDescriptor_StreamEndpoint_Spc_t audio;
    } ep1;
    struct usb_endpoint_descriptor_long ep2;
} audio_device_config;

static char *descriptor_strings[] = {
    "Ploopy Corporation",
    "Ploopy Headphones",
    "000000000001"
};

/*****************************************************************************
 * USB-related definitions end here.
 ****************************************************************************/

// I2C pin definitions
#define PCM3060_SDA_PIN 0
#define PCM3060_SCL_PIN 1

// PCM3060 pin definitions
#define PCM3060_SCKI2_PIN 19    // a.k.a. DAC SCKI
#define PCM3060_DAC_SCK_PIN 8   // a.k.a. DAC BCK
#define PCM3060_DAC_WS_PIN 9    // a.k.a. DAC LRCK
#define PCM3060_DAC_SD_PIN 12   // a.k.a. DAC DIN
#define PCM3060_RST_PIN 14      // a.k.a. PCM RESET

#define NEG_SWITCH_PWM_PIN 17

#define AUDIO_POS_SUPPLY_EN_PIN 22

#define PCM_I2C_ADDR 70

#define SYSTEM_FREQ 230400000
#define CODEC_FREQ 9216000
#define SAMPLING_FREQ (CODEC_FREQ / 192)

#define CORE0_READY 19813219
#define CORE1_READY 72965426

/*****************************************************************************
 * DO NOT CHANGE THESE VALUES. YOU COULD BREAK YOUR HARDWARE IF YOU DO!
 ****************************************************************************/
#define NEG_SWITCH_FREQ 3067000  // 1429000?
#define NEG_DUTY_NUM 34734       // 37451?
#define NEG_DUTY_DEN 65536
/*****************************************************************************
 * /seriousness
 ****************************************************************************/

void core1_entry(void);
void setup(void);
void configure_neg_switch_pwm(void);

const char *_get_descriptor_string(uint);
static void _as_sync_packet(struct usb_endpoint *);
static bool do_get_current(struct usb_setup_packet *);
static bool do_get_minimum(struct usb_setup_packet *);
static bool do_get_maximum(struct usb_setup_packet *);
static bool do_get_resolution(struct usb_setup_packet *);
static void _audio_reconfigure(void);
static void audio_set_volume(int16_t);
static void audio_cmd_packet(struct usb_endpoint *);
static bool as_set_alternate(struct usb_interface *, uint);
static bool do_set_current(struct usb_setup_packet *);
static bool ac_setup_request_handler(__unused struct usb_interface *, struct usb_setup_packet *);
bool _as_setup_request_handler(__unused struct usb_endpoint *, struct usb_setup_packet *);
void usb_sound_card_init(void);

#endif