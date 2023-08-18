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
 * @miketeachman (github.com/miketeachman)
 * @jimmo (github.com/jimmo)
 * @dlech (github.com/dlech)
 * for their exceptional work on the I2S library for the rp2 port of the
 * Micropython project (github.com/micropython/micropython).
 */

#ifndef I2S_H
#define I2S_H

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "ringbuf.h"

#define SAMPLES_PER_FRAME 2
#define PIO_INSTRUCTIONS_PER_BIT 2
#define RINGBUF_LEN_IN_BYTES 16384
#define I2S_NUM_DMA_CHANNELS 2

#define SIZEOF_DMA_BUFFER_IN_BYTES 768
#define SIZEOF_HALF_DMA_BUFFER_IN_BYTES (SIZEOF_DMA_BUFFER_IN_BYTES / 2)

typedef enum {
    GP_INPUT = 0,
    GP_OUTPUT = 1
} gpio_dir_t;

typedef struct _i2s_obj_t {
    uint sck_pin;
    uint ws_pin;
    uint sd_pin;
    PIO pio;
    uint8_t sm;
    float sampling_rate;
    const pio_program_t *pio_program;
    uint prog_offset;
    int dma_channel[I2S_NUM_DMA_CHANNELS];
    uint8_t dma_buffer[SIZEOF_DMA_BUFFER_IN_BYTES];
    ring_buf_t ring_buffer;
} i2s_obj_t;

extern i2s_obj_t i2s_write_obj;

void i2s_write_init(i2s_obj_t *);
uint i2s_stream_write(i2s_obj_t *, const uint32_t *, uint);

void dma_irq_handler(uint8_t);
void dma_irq_write_handler(void);
void gpio_init_i2s(PIO, uint8_t, uint, uint8_t, gpio_dir_t);
void dma_configure(i2s_obj_t *);
uint8_t *dma_get_buffer(i2s_obj_t *, uint);
void feed_dma(i2s_obj_t *, uint8_t *);

uint32_t copy_userbuf_to_ringbuf(i2s_obj_t *, const uint32_t *, uint);

#endif