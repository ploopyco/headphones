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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "pico/stdlib.h"

#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"

#include "ringbuf.h"
#include "i2s.h"
#include "i2s.pio.h"

void i2s_write_init(i2s_obj_t *self) {
    self->pio = pio1;
    self->pio_program = &i2s_write_program;
    self->sm = pio_claim_unused_sm(self->pio, true);
    self->prog_offset = pio_add_program(self->pio, self->pio_program);
    pio_sm_init(self->pio, self->sm, self->prog_offset, NULL);

    pio_sm_config config = pio_get_default_sm_config();

    float pio_freq = self->sampling_rate * SAMPLES_PER_FRAME * 32 *
            PIO_INSTRUCTIONS_PER_BIT;

    float clkdiv = clock_get_hz(clk_sys) / pio_freq;

    sm_config_set_clkdiv(&config, clkdiv);

    sm_config_set_out_pins(&config, self->sd_pin, 1);
    sm_config_set_out_shift(&config, false, true, 32);
    sm_config_set_fifo_join(&config, PIO_FIFO_JOIN_TX);  // double TX FIFO size

    sm_config_set_sideset(&config, 2, false, false);
    sm_config_set_sideset_pins(&config, self->sck_pin);
    sm_config_set_wrap(&config, self->prog_offset,
            self->prog_offset + self->pio_program->length - 1);
    pio_sm_set_config(self->pio, self->sm, &config);

    uint8_t *rbs = malloc(sizeof(uint8_t) * RINGBUF_LEN_IN_BYTES);
    ringbuf_init(&self->ring_buffer, rbs, RINGBUF_LEN_IN_BYTES);

    irq_set_exclusive_handler(DMA_IRQ_1, dma_irq_write_handler);
    irq_set_enabled(DMA_IRQ_1, true);

    gpio_init_i2s(self->pio, self->sm, self->sck_pin, 0, GP_OUTPUT);
    gpio_init_i2s(self->pio, self->sm, self->ws_pin, 0, GP_OUTPUT);
    gpio_init_i2s(self->pio, self->sm, self->sd_pin, 0, GP_OUTPUT);

    dma_configure(self);

    pio_sm_set_enabled(self->pio, self->sm, true);
    dma_channel_start(self->dma_channel[0]);
}

void gpio_init_i2s(PIO pio, uint8_t sm, uint pin_num, uint8_t pin_val, gpio_dir_t pin_dir) {
    uint32_t pinmask = 1 << pin_num;
    pio_sm_set_pins_with_mask(pio, sm, pin_val << pin_num, pinmask);
    pio_sm_set_pindirs_with_mask(pio, sm, pin_dir << pin_num, pinmask);
    pio_gpio_init(pio, pin_num);
}

void dma_irq_write_handler() {
    i2s_obj_t *self = &i2s_write_obj;

    uint dma_channel;
    if (dma_irqn_get_channel_status(1, self->dma_channel[0]))
        dma_channel = self->dma_channel[0];
    else if (dma_irqn_get_channel_status(1, self->dma_channel[1]))
        dma_channel = self->dma_channel[1];
    else {
        //printf("ERROR write: dma_channel not found");
        exit(1);
    }

    uint8_t *dma_buffer = dma_get_buffer(self, dma_channel);
    if (dma_buffer == NULL) {
        //printf("ERROR write: dma_buffer not found\n");
        exit(1);
    }

    feed_dma(self, dma_buffer);
    dma_irqn_acknowledge_channel(1, dma_channel);
    dma_channel_set_read_addr(dma_channel, dma_buffer, false);
}

void dma_configure(i2s_obj_t *self) {
    uint8_t num_free_dma_channels = 0;
    for (uint8_t ch = 0; ch < NUM_DMA_CHANNELS; ch++) {
        if (!dma_channel_is_claimed(ch)) {
            num_free_dma_channels++;
        }
    }
    if (num_free_dma_channels < I2S_NUM_DMA_CHANNELS) {
        //printf("ERROR: cannot claim 2 DMA channels");
        exit(1);
    }

    for (uint8_t ch = 0; ch < I2S_NUM_DMA_CHANNELS; ch++) {
        self->dma_channel[ch] = dma_claim_unused_channel(true);
    }

    // The DMA channels are chained together.  The first DMA channel is used to access
    // the top half of the DMA buffer.  The second DMA channel accesses the bottom half of the DMA buffer.
    // With chaining, when one DMA channel has completed a data transfer, the other
    // DMA channel automatically starts a new data transfer.
    enum dma_channel_transfer_size dma_size = DMA_SIZE_32;
    for (uint8_t ch = 0; ch < I2S_NUM_DMA_CHANNELS; ch++) {
        dma_channel_config dma_config = dma_channel_get_default_config(self->dma_channel[ch]);
        channel_config_set_transfer_data_size(&dma_config, dma_size);
        channel_config_set_chain_to(&dma_config, self->dma_channel[(ch + 1) % I2S_NUM_DMA_CHANNELS]);

        uint8_t *dma_buffer = self->dma_buffer + (SIZEOF_HALF_DMA_BUFFER_IN_BYTES * ch);
        channel_config_set_dreq(&dma_config, pio_get_dreq(self->pio, self->sm, true));
        channel_config_set_read_increment(&dma_config, true);
        channel_config_set_write_increment(&dma_config, false);
        dma_channel_configure(self->dma_channel[ch],
            &dma_config,
            (void *)&self->pio->txf[self->sm],                      // dest = PIO TX FIFO
            dma_buffer,                                             // src = DMA buffer
            SIZEOF_HALF_DMA_BUFFER_IN_BYTES / (32 / 8),
            false);
    }

    for (uint8_t ch = 0; ch < I2S_NUM_DMA_CHANNELS; ch++) {
        dma_irqn_acknowledge_channel(1, self->dma_channel[ch]);  // clear pending.  e.g. from SPI
        dma_irqn_set_channel_enabled(1, self->dma_channel[ch], true);
    }
}

// note:  first DMA channel is mapped to the top half of buffer, second is mapped to the bottom half
uint8_t *dma_get_buffer(i2s_obj_t *i2s_obj, uint channel) {
    for (uint8_t ch = 0; ch < I2S_NUM_DMA_CHANNELS; ch++) {
        if (i2s_obj->dma_channel[ch] == channel) {
            return i2s_obj->dma_buffer + (SIZEOF_HALF_DMA_BUFFER_IN_BYTES * ch);
        }
    }
    // This should never happen
    return NULL;
}

void feed_dma(i2s_obj_t *self, uint8_t *dma_buffer_p) {
    // when data exists, copy samples from ring buffer
    if (ringbuf_available_data(&self->ring_buffer) >= SIZEOF_HALF_DMA_BUFFER_IN_BYTES) {
        for (uint32_t i = 0; i < SIZEOF_HALF_DMA_BUFFER_IN_BYTES; i++)
            ringbuf_pop(&self->ring_buffer, &dma_buffer_p[i]);
    } else {
        // underflow.  clear buffer to transmit "silence" on the I2S bus
        memset(dma_buffer_p, 0, SIZEOF_HALF_DMA_BUFFER_IN_BYTES);
    }
}

uint i2s_stream_write(i2s_obj_t *self, const uint8_t *buf_out, uint size) {
    if (size == 0) {
        //printf("ERROR: buffer can't be length zero");
        exit(1);
    }

    uint32_t num_bytes_written = copy_userbuf_to_ringbuf(self, buf_out, size);
    return num_bytes_written;
}

// TODO maybe we can skip every fourth byte, if we're doing this in 24-bit...
// could save on some processing power
uint32_t copy_userbuf_to_ringbuf(i2s_obj_t *self, const uint8_t *buf_out, uint size) {
    uint32_t a_index = 0;

    while (a_index < size) {
        // copy a byte to the ringbuf when space becomes available
        while (ringbuf_push(&self->ring_buffer, buf_out[a_index]) == false) {
            ;
        }
        a_index++;
    }

    return a_index;
}
