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
#ifndef CONFIGURATION_MANAGER_H
#define CONFIGURATION_MANAGER_H
struct usb_endpoint;

// TODO: Duplicated from os_descriptors.h
#define U16_HIGH(_u16)          ((uint8_t) (((_u16) >> 8) & 0x00ff))
#define U16_LOW(_u16)           ((uint8_t) ((_u16)       & 0x00ff))

#define U16_TO_U8S_LE(_u16)     U16_LOW(_u16), U16_HIGH(_u16)

#define INIT_FILTER2(T) { \
    filter2 *args = (filter2 *)ptr; \
    bqf_##T##_config(SAMPLING_FREQ, args->f0, args->Q, &bqf_filters_left[filter_stages]); \
    memcpy(&bqf_filters_right[filter_stages], &bqf_filters_left[filter_stages], sizeof(bqf_coeff_t)); \
    ptr += sizeof(filter2); \
    break; \
    }

#define INIT_FILTER3(T) { \
    filter3 *args = (filter3 *)ptr; \
    bqf_##T##_config(SAMPLING_FREQ, args->f0, args->db_gain, args->Q, &bqf_filters_left[filter_stages]); \
    memcpy(&bqf_filters_right[filter_stages], &bqf_filters_left[filter_stages], sizeof(bqf_coeff_t)); \
    ptr += sizeof(filter3); \
    break; \
    }

void config_in_packet(struct usb_endpoint *ep);
void config_out_packet(struct usb_endpoint *ep);
void configuration_ep_on_stall_change(struct usb_endpoint *ep);
void configuration_ep_on_cancel(struct usb_endpoint *ep);
extern void load_config();
extern void apply_core1_config();

#endif // CONFIGURATION_MANAGER_H