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

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "hardware/vreg.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include "hardware/sync.h"

#include "pico/stdlib.h"
#include "pico/usb_device.h"
#include "pico/multicore.h"
#include "pico/bootrom.h"
#include "AudioClassCommon.h"

#include "run.h"
#include "ringbuf.h"
#include "i2s.h"
#include "bqf.h"
#include "user.h"

i2s_obj_t i2s_write_obj;
static uint8_t *userbuf;

bqf_coeff_t bqf_filters_left[MAX_FILTER_STAGES];
bqf_coeff_t bqf_filters_right[MAX_FILTER_STAGES];
bqf_mem_t bqf_filters_mem_left[MAX_FILTER_STAGES];
bqf_mem_t bqf_filters_mem_right[MAX_FILTER_STAGES];

static struct {
    uint32_t freq;
    union {
        int16_t volume[2];
        int32_t _volume;
    };
    union {
        int16_t target_volume[2];
        int32_t _target_volume;
    };
    bool mute;
} audio_state = {
    .freq = 48000,
};

enum vendor_cmds {
    REBOOT_BOOTLOADER = 0
};

int main(void) {
    setup();

    define_filters();

    // start second core (called "core 1" in the SDK)
    multicore_launch_core1(core1_entry);
    multicore_fifo_push_blocking((uintptr_t) userbuf);
    uint32_t ready = multicore_fifo_pop_blocking();
    if (ready != CORE1_READY) {
        //printf("core 1 startup sequence is hella borked")
        exit(1);
    }

    usb_sound_card_init();

    while (true)
        __wfi();
}

// Here's the meat. It's where the data buffer from USB gets transformed from
// PCM data into I2S data that gets shipped out to the PCM3060. It really
// belongs with the other USB-related code due to its utter indecipherability,
// but it's placed here to emphasize its importance.
static void _as_audio_packet(struct usb_endpoint *ep) {
    struct usb_buffer *usb_buffer = usb_current_out_packet_buffer(ep);
    int16_t *in = (int16_t *) usb_buffer->data;
    int32_t *out = (int32_t *) userbuf;
    int samples = usb_buffer->data_len / 2;

    for (int i = 0; i < samples; i++)
        out[i] = in[i];

    multicore_fifo_push_blocking(CORE0_READY);
    multicore_fifo_push_blocking(samples);

    for (int j = 0; j < FILTER_STAGES; j++) {
        // Left channel filter
        for (int i = 0; i < samples; i += 2) {
            fix16_t x_f16 = fix16_from_int((int16_t) out[i]);

            x_f16 = bqf_transform(x_f16, &bqf_filters_left[j],
                &bqf_filters_mem_left[j]);

            out[i] = (int32_t) fix16_to_int(x_f16);
        }
    }

    // Block until core 1 has finished transforming the data
    uint32_t ready = multicore_fifo_pop_blocking();

    i2s_stream_write(&i2s_write_obj, userbuf, samples * 4);

    // keep on truckin'
    usb_grow_transfer(ep->current_transfer, 1);
    usb_packet_done(ep);
}

void core1_entry() {
    uint8_t *userbuf = (uint8_t *) multicore_fifo_pop_blocking();
    int32_t *out = (int32_t *) userbuf;

    multicore_fifo_push_blocking(CORE1_READY);

    while (true) {
        // Block until the userbuf is filled with data
        uint32_t ready = multicore_fifo_pop_blocking();
        while (ready != CORE0_READY)
            ready = multicore_fifo_pop_blocking();
        
        uint32_t limit = multicore_fifo_pop_blocking();

        for (int j = 0; j < FILTER_STAGES; j++) {
            for (int i = 1; i < limit; i += 2) {
                fix16_t x_f16 = fix16_from_int((int16_t) out[i]);

                x_f16 = bqf_transform(x_f16, &bqf_filters_right[j],
                    &bqf_filters_mem_right[j]);

                out[i] = (int16_t) fix16_to_int(x_f16);
            }
        }

        // Signal to core 0 that the data has all been transformed
        multicore_fifo_push_blocking(CORE1_READY);

        // Update the volume if required. We do this from core1 as
        // core0 is more heavily loaded, doing this from core0 can
        // lead to audio crackling.
        if (audio_state._volume != audio_state._target_volume) {
            // PCM3060 volume attenuation:
            //  0: 0db (default)
            //  55: -100db
            //  56..: Mute
            uint8_t buf[3];
            buf[0] = 65;    // register addr
            buf[1] = 255 + (audio_state.target_volume[0] / 128); // data left
            buf[2] = 255 + (audio_state.target_volume[1] / 128); // data right
            i2c_write_blocking(i2c0, PCM_I2C_ADDR, buf, 3, false);

            audio_state._volume = audio_state._target_volume;
        }
    }
}

void setup() {
    set_sys_clock_khz(SYSTEM_FREQ / 1000, true);
    sleep_ms(100);
    // stdio_init_all();

    userbuf = malloc(sizeof(uint8_t) * RINGBUF_LEN_IN_BYTES);
    
    // Configure DAC PWM
    gpio_set_function(PCM3060_SCKI2_PIN, GPIO_FUNC_PWM);
    uint slice_num_dac = pwm_gpio_to_slice_num(PCM3060_SCKI2_PIN);
    uint chan_num_dac = pwm_gpio_to_channel(PCM3060_SCKI2_PIN);
    pwm_set_phase_correct(slice_num_dac, false);
    pwm_set_wrap(slice_num_dac, (SYSTEM_FREQ / CODEC_FREQ) - 1);
    pwm_set_chan_level(slice_num_dac, chan_num_dac, (SYSTEM_FREQ / CODEC_FREQ / 2));
    pwm_set_enabled(slice_num_dac, true);

    gpio_init(AUDIO_POS_SUPPLY_EN_PIN);
    gpio_set_dir(AUDIO_POS_SUPPLY_EN_PIN, GPIO_OUT);
    gpio_put(AUDIO_POS_SUPPLY_EN_PIN, true);

    sleep_ms(100);

    configure_neg_switch_pwm();

    // After negative switching PWM is configured, take the PCM out of reset
    gpio_init(PCM3060_RST_PIN);
    gpio_set_dir(PCM3060_RST_PIN, GPIO_OUT);
    gpio_put(PCM3060_RST_PIN, true);

    // The PCM3060 supports standard mode (100kbps) or fast mode (400kbps)
    // we run in fast mode so we dont block the core for too long while
    // updating the volume.
    i2c_init(i2c0, 400000);
    gpio_set_function(PCM3060_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PCM3060_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PCM3060_SDA_PIN);
    gpio_pull_up(PCM3060_SCL_PIN);

    // Let the PCM stabilize before power-on
    sleep_ms(200);

    // Resynchronise clocks. Do not enable PCM yet.
    uint8_t buf[2];
    buf[0] = 64; // register addr
    buf[1] = 0xB0; // data
    i2c_write_blocking(i2c0, PCM_I2C_ADDR, buf, 2, false);

    // Don't remove this. Don't do it.
    sleep_ms(200);

    // Set data format to 16 bit right justified, MSB first
    buf[0] = 67;   // register addr
    buf[1] = 0x03; // data
    i2c_write_blocking(i2c0, PCM_I2C_ADDR, buf, 2, false);

    // Enable DAC
    buf[0] = 64; // register addr
    buf[1] = 0xE0; // data
    i2c_write_blocking(i2c0, PCM_I2C_ADDR, buf, 2, false);

    // Same here, pal. Hands off.
    sleep_ms(100);

    i2s_write_obj.sck_pin = PCM3060_DAC_SCK_PIN;
    i2s_write_obj.ws_pin = PCM3060_DAC_WS_PIN;
    i2s_write_obj.sd_pin = PCM3060_DAC_SD_PIN;
    i2s_write_obj.sampling_rate = SAMPLING_FREQ;

    i2s_write_init(&i2s_write_obj);
}

/** **************************************************************************
 * DO. NOT. FUCKING. CHANGE. THIS. FUNCTION.                                 *
 * IF YOU DO, YOU COULD BLOW UP YOUR HARDWARE!                               *
 * YOU WERE WARNED!!!!!!!!!!!!!!!!                                           *
 ****************************************************************************/
void configure_neg_switch_pwm() {
    gpio_set_function(NEG_SWITCH_PWM_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(NEG_SWITCH_PWM_PIN);
    uint chan_num = pwm_gpio_to_channel(NEG_SWITCH_PWM_PIN);
    pwm_set_phase_correct(slice_num, false);

    uint16_t wrap = round((float) SYSTEM_FREQ / (float) NEG_SWITCH_FREQ);
    pwm_set_wrap(slice_num, wrap - 1);

    uint16_t target_level = round((float) SYSTEM_FREQ / (float) NEG_SWITCH_FREQ /
            (float) NEG_DUTY_DEN * (float) NEG_DUTY_NUM);
    pwm_set_chan_level(slice_num, chan_num, 0);
    pwm_set_enabled(slice_num, true);
    sleep_ms(10);

    // Ramp up the duty cycle.
    // Seriously, don't fuck with this. A spike on the negative voltage supply
    // because this isn't ramping correctly will destroy the hardware.
    size_t i;
    for(i = 0; i < 200; i++) {
        uint16_t current_level = round(i * ((float)target_level / 200.0));
        pwm_set_chan_level(slice_num, chan_num, current_level);
        sleep_ms(1);
    }
}




/*****************************************************************************
 * USB-related code begins here. It's a refactoring nightmare, so here it
 * shall lie for a thousand years.
 ****************************************************************************/

// todo noop when muted

static const audio_device_config ad_conf = {
    .descriptor = {
        .bLength = sizeof(ad_conf.descriptor),
        .bDescriptorType = DTYPE_Configuration,
        .wTotalLength = sizeof(ad_conf),
        .bNumInterfaces = 2,
        .bConfigurationValue = 0x01,
        .iConfiguration = 0x00,
        .bmAttributes = 0x80,
        .bMaxPower = 0xFA,
    },
    .ac_interface = {
        .bLength = sizeof(ad_conf.ac_interface),
        .bDescriptorType = DTYPE_Interface,
        .bInterfaceNumber = 0x00,
        .bAlternateSetting = 0x00,
        .bNumEndpoints = 0x00,
        .bInterfaceClass = AUDIO_CSCP_AudioClass,
        .bInterfaceSubClass = AUDIO_CSCP_ControlSubclass,
        .bInterfaceProtocol = AUDIO_CSCP_ControlProtocol,
        .iInterface = 0x00,
    },
    .ac_audio = {
        .core = {
            .bLength = sizeof(ad_conf.ac_audio.core),
            .bDescriptorType = AUDIO_DTYPE_CSInterface,
            .bDescriptorSubtype = AUDIO_DSUBTYPE_CSInterface_Header,
            .bcdADC = VERSION_BCD(1, 0, 0),
            .wTotalLength = sizeof(ad_conf.ac_audio),
            .bInCollection = 1,
            .bInterfaceNumbers = 1,
        },
        .input_terminal = {
            .bLength = sizeof(ad_conf.ac_audio.input_terminal),
            .bDescriptorType = AUDIO_DTYPE_CSInterface,
            .bDescriptorSubtype = AUDIO_DSUBTYPE_CSInterface_InputTerminal,
            .bTerminalID = 1,
            .wTerminalType = AUDIO_TERMINAL_STREAMING,
            .bAssocTerminal = 0,
            .bNrChannels = 2,
            .wChannelConfig = AUDIO_CHANNEL_LEFT_FRONT | AUDIO_CHANNEL_RIGHT_FRONT,
            .iChannelNames = 0,
            .iTerminal = 0,
        },
        .feature_unit = {
            .bLength = sizeof(ad_conf.ac_audio.feature_unit),
            .bDescriptorType = AUDIO_DTYPE_CSInterface,
            .bDescriptorSubtype = AUDIO_DSUBTYPE_CSInterface_Feature,
            .bUnitID = 2,
            .bSourceID = 1,
            .bControlSize = 1,
            .bmaControls = {
                AUDIO_FEATURE_MUTE, // Master channel
                AUDIO_FEATURE_VOLUME, // Left channel
                AUDIO_FEATURE_VOLUME, // Right channel
            },
            .iFeature = 0,
        },
        .output_terminal = {
            .bLength = sizeof(ad_conf.ac_audio.output_terminal),
            .bDescriptorType = AUDIO_DTYPE_CSInterface,
            .bDescriptorSubtype = AUDIO_DSUBTYPE_CSInterface_OutputTerminal,
            .bTerminalID = 3,
            .wTerminalType = AUDIO_TERMINAL_OUT_HEADPHONES,
            .bAssocTerminal = 0,
            .bSourceID = 2,
            .iTerminal = 0,
        },
    },
    .as_zero_interface = {
        .bLength = sizeof(ad_conf.as_zero_interface),
        .bDescriptorType = DTYPE_Interface,
        .bInterfaceNumber = 0x01,
        .bAlternateSetting = 0x00,
        .bNumEndpoints = 0x00,
        .bInterfaceClass = AUDIO_CSCP_AudioClass,
        .bInterfaceSubClass = AUDIO_CSCP_AudioStreamingSubclass,
        .bInterfaceProtocol = AUDIO_CSCP_ControlProtocol,
        .iInterface = 0x00,
    },
    .as_op_interface = {
        .bLength = sizeof(ad_conf.as_op_interface),
        .bDescriptorType = DTYPE_Interface,
        .bInterfaceNumber = 0x01,
        .bAlternateSetting = 0x01,
        .bNumEndpoints = 0x02,
        .bInterfaceClass = AUDIO_CSCP_AudioClass,
        .bInterfaceSubClass = AUDIO_CSCP_AudioStreamingSubclass,
        .bInterfaceProtocol = AUDIO_CSCP_ControlProtocol,
        .iInterface = 0x00,
    },
    .as_audio = {
        .streaming = {
            .bLength = sizeof(ad_conf.as_audio.streaming),
            .bDescriptorType = AUDIO_DTYPE_CSInterface,
            .bDescriptorSubtype = AUDIO_DSUBTYPE_CSInterface_General,
            .bTerminalLink = 1,
            .bDelay = 1,
            .wFormatTag = 1, // PCM
        },
        .format = {
            .core = {
                .bLength = sizeof(ad_conf.as_audio.format),
                .bDescriptorType = AUDIO_DTYPE_CSInterface,
                .bDescriptorSubtype = AUDIO_DSUBTYPE_CSInterface_FormatType,
                .bFormatType = 1,
                .bNrChannels = 2,
                .bSubFrameSize = 2,
                .bBitResolution = 16,
                .bSampleFrequencyType = 1,
            },
            .freqs = {
                0x80,
                0xBB,
                0x00
            },
        },
    },
    .ep1 = {
        .core = {
            .bLength = sizeof(ad_conf.ep1.core),
            .bDescriptorType = DTYPE_Endpoint,
            .bEndpointAddress = 0x01,
            .bmAttributes = 5,
            .wMaxPacketSize = (uint8_t) 0xC4,
            .bInterval = 1,
            .bRefresh = 0,
            .bSyncAddr = 0x82,
        },
        .audio = {
            .bLength = sizeof(ad_conf.ep1.audio),
            .bDescriptorType = AUDIO_DTYPE_CSEndpoint,
            .bDescriptorSubtype = AUDIO_DSUBTYPE_CSEndpoint_General,
            .bmAttributes = 1,
            .bLockDelayUnits = 0,
            .wLockDelay = 0,
        }
    },
    .ep2 = {
        .bLength = sizeof(ad_conf.ep2),
        .bDescriptorType = 0x05,
        .bEndpointAddress = 0x82,
        .bmAttributes = 0x11,
        .wMaxPacketSize = 3,
        .bInterval = 0x01,
        .bRefresh = 2,
        .bSyncAddr = 0,
    },
};

static struct usb_interface ac_interface;
static struct usb_interface as_op_interface;
static struct usb_endpoint ep_op_out, ep_op_sync;

static const struct usb_device_descriptor boot_device_descriptor = {
    .bLength            = 18,
    .bDescriptorType    = 0x01,
    .bcdUSB             = 0x0110,
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = 0x40,
    .idVendor           = 0x2E8A,
    .idProduct          = 0xFEDD,
    .bcdDevice          = 0x0200,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
    .bNumConfigurations = 0x01,
};

const char *_get_descriptor_string(uint index) {
    if (index <= count_of(descriptor_strings)) {
        return descriptor_strings[index - 1];
    } else {
        return "";
    }
}

static void _as_sync_packet(struct usb_endpoint *ep) {
    assert(ep->current_transfer);
    struct usb_buffer *buffer = usb_current_in_packet_buffer(ep);
    assert(buffer->data_max >= 3);
    buffer->data_len = 3;

    ring_buf_t rb = i2s_write_obj.ring_buffer;
    uint32_t feedback;

    size_t lower_limit = (RINGBUF_LEN_IN_BYTES / 2) - (RINGBUF_LEN_IN_BYTES / 4);
    size_t upper_limit = (RINGBUF_LEN_IN_BYTES / 2) + (RINGBUF_LEN_IN_BYTES / 4);

    if (ringbuf_available_data(&rb) > upper_limit) {
        // slow down
        feedback = 47 << 14;
    } else if (ringbuf_available_data(&rb) < lower_limit) {
        // we need more data
        feedback = 49 << 14;
    } else
        feedback = 48 << 14;

    //double temp = rate * 0x00004000;
    //temp += (double)((temp >= 0) ? 0.5f : -0.5f);
    //uint32_t feedback = (uint32_t) temp;

    // todo lie thru our teeth for now
    //uint feedback = 48 << 14u;

    buffer->data[0] = feedback;
    buffer->data[1] = feedback >> 8u;
    buffer->data[2] = feedback >> 16u;

    // keep on truckin'
    usb_grow_transfer(ep->current_transfer, 1);
    usb_packet_done(ep);
}

static const struct usb_transfer_type as_transfer_type = {
    .on_packet = _as_audio_packet,
    .initial_packet_count = 1,
};

static const struct usb_transfer_type as_sync_transfer_type = {
    .on_packet = _as_sync_packet,
    .initial_packet_count = 1,
};

static struct usb_transfer as_transfer;
static struct usb_transfer as_sync_transfer;

static bool do_get_current(struct usb_setup_packet *setup) {
    if ((setup->bmRequestType & USB_REQ_TYPE_RECIPIENT_MASK) == USB_REQ_TYPE_RECIPIENT_INTERFACE) {
        switch (setup->wValue >> 8u) {
            case 1: { // mute
                usb_start_tiny_control_in_transfer(audio_state.mute, 1);
                return true;
            }
            case 2: { // volume
                /* Current volume. See UAC Spec 1.0 p.77 */
                const uint8_t cn = (uint8_t) setup->wValue;
                if (cn == AUDIO_CHANNEL_LEFT_FRONT) {
                    usb_start_tiny_control_in_transfer(audio_state.target_volume[0], 2);
                }
                else if (cn == AUDIO_CHANNEL_RIGHT_FRONT) {
                    usb_start_tiny_control_in_transfer(audio_state.target_volume[1], 2);
                }
                else
                {
                    return false;
                }
                return true;
            }
        }
    } else if ((setup->bmRequestType & USB_REQ_TYPE_RECIPIENT_MASK) == USB_REQ_TYPE_RECIPIENT_ENDPOINT) {
        if ((setup->wValue >> 8u) == 1) { // endpoint frequency control
            /* Current frequency */
            usb_start_tiny_control_in_transfer(audio_state.freq, 3);
            return true;
        }
    }
    return false;
}

static bool do_get_minimum(struct usb_setup_packet *setup) {
    if ((setup->bmRequestType & USB_REQ_TYPE_RECIPIENT_MASK) == USB_REQ_TYPE_RECIPIENT_INTERFACE) {
        switch (setup->wValue >> 8u) {
            case 2: { // volume
                usb_start_tiny_control_in_transfer(MIN_VOLUME, 2);
                return true;
            }
        }
    }
    return false;
}

static bool do_get_maximum(struct usb_setup_packet *setup) {
    if ((setup->bmRequestType & USB_REQ_TYPE_RECIPIENT_MASK) == USB_REQ_TYPE_RECIPIENT_INTERFACE) {
        switch (setup->wValue >> 8u) {
            case 2: { // volume
                usb_start_tiny_control_in_transfer(MAX_VOLUME, 2);
                return true;
            }
        }
    }
    return false;
}

static bool do_get_resolution(struct usb_setup_packet *setup) {
    if ((setup->bmRequestType & USB_REQ_TYPE_RECIPIENT_MASK) == USB_REQ_TYPE_RECIPIENT_INTERFACE) {
        switch (setup->wValue >> 8u) {
            case 2: { // volume
                usb_start_tiny_control_in_transfer(VOLUME_RESOLUTION, 2);
                return true;
            }
        }
    }
    return false;
}

static struct audio_control_cmd {
    uint8_t cmd;
    uint8_t type;
    uint8_t cs;
    uint8_t cn;
    uint8_t unit;
    uint8_t len;
} audio_control_cmd_t;

static void _audio_reconfigure() {
    switch (audio_state.freq) {
        case 44100:
        case 48000:
            break;
        default:
            audio_state.freq = 48000;
    }
}

static void audio_set_volume(int8_t channel, int16_t volume) {
    // volume is in the range 127.9961dB (0x7FFF) .. -127.9961dB (0x8001). 0x8000 = mute
    // the old code reported a min..max volume of -90.9961dB (0xA500) .. 0dB (0x0)

    if (volume == 0x8000) {
        // Mute case
    }
    else if (volume > (int16_t) MAX_VOLUME) {
        volume = MAX_VOLUME;
    }
    else if (volume < (int16_t) MIN_VOLUME) {
        volume = MIN_VOLUME;
    }
    if (channel == AUDIO_CHANNEL_LEFT_FRONT || channel == 0) {
        audio_state.target_volume[0] = volume;
    }
    if (channel == AUDIO_CHANNEL_RIGHT_FRONT || channel == 0) {
        audio_state.target_volume[1] = volume;
    }
}

static void audio_cmd_packet(struct usb_endpoint *ep) {
    assert(audio_control_cmd_t.cmd == AUDIO_REQ_SetCurrent);
    struct usb_buffer *buffer = usb_current_out_packet_buffer(ep);

    // printf("%s: CMD: %u, Type: %u, CS: %u, CN: %u, Unit: %u, Len: %u\n", __PRETTY_FUNCTION__, audio_control_cmd_t.cmd, audio_control_cmd_t.type,
    //    audio_control_cmd_t.cs, audio_control_cmd_t.cn, audio_control_cmd_t.unit, audio_control_cmd_t.len);

    audio_control_cmd_t.cmd = 0;
    if (buffer->data_len >= audio_control_cmd_t.len) {
        if (audio_control_cmd_t.type == USB_REQ_TYPE_RECIPIENT_INTERFACE) {
            switch (audio_control_cmd_t.cs) {
                case 1: { // mute
                    audio_state.mute = buffer->data[0];
                    uint8_t buf[2];
                    buf[0] = 68;   // register addr
                    buf[1] = buffer->data[0] ? 0x3 : 0x0; // data
                    i2c_write_blocking(i2c0, PCM_I2C_ADDR, buf, 2, false);
                    break;
                }
                case 2: { // volume
                    audio_set_volume(audio_control_cmd_t.cn, *(int16_t *) buffer->data);
                    break;
                }
            }

        } else if (audio_control_cmd_t.type == USB_REQ_TYPE_RECIPIENT_ENDPOINT) {
            if (audio_control_cmd_t.cs == 1) { // endpoint frequency control
                uint32_t new_freq = (*(uint32_t *) buffer->data) & 0x00ffffffu;

                if (audio_state.freq != new_freq) {
                    audio_state.freq = new_freq;
                    _audio_reconfigure();
                }
            }
        }
    }
    usb_start_empty_control_in_transfer_null_completion();
    // todo is there error handling?
}


static const struct usb_transfer_type _audio_cmd_transfer_type = {
    .on_packet = audio_cmd_packet,
    .initial_packet_count = 1,
};

static bool as_set_alternate(struct usb_interface *interface, uint alt) {
    assert(interface == &as_op_interface);
    return alt < 2;
}

static bool do_set_current(struct usb_setup_packet *setup) {
    if (setup->wLength && setup->wLength < 64) {
        audio_control_cmd_t.cmd = AUDIO_REQ_SetCurrent;
        audio_control_cmd_t.type = setup->bmRequestType & USB_REQ_TYPE_RECIPIENT_MASK;
        audio_control_cmd_t.len = (uint8_t) setup->wLength;
        audio_control_cmd_t.unit = setup->wIndex >> 8u;
        audio_control_cmd_t.cs = setup->wValue >> 8u;
        audio_control_cmd_t.cn = (uint8_t) setup->wValue;
        usb_start_control_out_transfer(&_audio_cmd_transfer_type);
        return true;
    }
    return false;
}

static bool ad_setup_request_handler(__unused struct usb_device *device, struct usb_setup_packet *setup) {
    setup = __builtin_assume_aligned(setup, 4);
    if (USB_REQ_TYPE_TYPE_VENDOR == (setup->bmRequestType & USB_REQ_TYPE_TYPE_MASK)) {
        // To prevent badly behaving software from accidentally triggering a reboot, e expect
        // the wValue to be equal to the Ploopy vendor id.
        if (setup->bRequest == REBOOT_BOOTLOADER && setup->wValue == 0x2E8A) {
            reset_usb_boot(0, 0);
            // reset_usb_boot does not return, so we will not respond to this command.
            return true;
        }
    }
    return false;
}

static bool ac_setup_request_handler(__unused struct usb_interface *interface, struct usb_setup_packet *setup) {
    setup = __builtin_assume_aligned(setup, 4);
    if (USB_REQ_TYPE_TYPE_CLASS == (setup->bmRequestType & USB_REQ_TYPE_TYPE_MASK)) {
        switch (setup->bRequest) {
            case AUDIO_REQ_SetCurrent:
                return do_set_current(setup);

            case AUDIO_REQ_GetCurrent:
                return do_get_current(setup);

            case AUDIO_REQ_GetMinimum:
                return do_get_minimum(setup);

            case AUDIO_REQ_GetMaximum:
                return do_get_maximum(setup);

            case AUDIO_REQ_GetResolution:
                return do_get_resolution(setup);

            default:
                break;
        }
    }
    return false;
}

bool _as_setup_request_handler(__unused struct usb_endpoint *ep, struct usb_setup_packet *setup) {
    setup = __builtin_assume_aligned(setup, 4);
    if (USB_REQ_TYPE_TYPE_CLASS == (setup->bmRequestType & USB_REQ_TYPE_TYPE_MASK)) {
        switch (setup->bRequest) {
            case AUDIO_REQ_SetCurrent:
                return do_set_current(setup);

            case AUDIO_REQ_GetCurrent:
                return do_get_current(setup);

            case AUDIO_REQ_GetMinimum:
                return do_get_minimum(setup);

            case AUDIO_REQ_GetMaximum:
                return do_get_maximum(setup);

            case AUDIO_REQ_GetResolution:
                return do_get_resolution(setup);

            default:
                break;
        }
    }
    return false;
}

void usb_sound_card_init() {
    usb_interface_init(&ac_interface, &ad_conf.ac_interface, NULL, 0, true);
    ac_interface.setup_request_handler = ac_setup_request_handler;

    static struct usb_endpoint *const op_endpoints[] = {
        &ep_op_out, &ep_op_sync
    };

    usb_interface_init(&as_op_interface, &ad_conf.as_op_interface, op_endpoints,
        count_of(op_endpoints), true);
    
    as_op_interface.set_alternate_handler = as_set_alternate;
    ep_op_out.setup_request_handler = _as_setup_request_handler;
    as_transfer.type = &as_transfer_type;
    usb_set_default_transfer(&ep_op_out, &as_transfer);
    as_sync_transfer.type = &as_sync_transfer_type;
    usb_set_default_transfer(&ep_op_sync, &as_sync_transfer);

    static struct usb_interface *const boot_device_interfaces[] = {
        &ac_interface,
        &as_op_interface,
    };

    __unused struct usb_device *device = usb_device_init(&boot_device_descriptor,
        &ad_conf.descriptor, boot_device_interfaces,
        count_of(boot_device_interfaces), _get_descriptor_string);

    assert(device);

    device->setup_request_handler = ad_setup_request_handler;
    audio_set_volume(0, DEFAULT_VOLUME);
    _audio_reconfigure();

    usb_device_start();
}

/*****************************************************************************
 * USB-related code ends here.
 ****************************************************************************/
