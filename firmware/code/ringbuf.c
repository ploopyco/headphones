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

#include "pico/stdlib.h"
#include "ringbuf.h"

// TODO can this be sped up by using uint32_t instead of uint8_t?

// Ring Buffer
// Thread safe when used with these constraints:
// - Single Producer, Single Consumer
// - Sequential atomic operations
// One byte of capacity is used to detect buffer empty/full

void ringbuf_init(ring_buf_t *rbuf, uint8_t *buffer, size_t size) {
    rbuf->buffer = buffer;
    rbuf->size = size;
    rbuf->head = 0;
    rbuf->tail = 0;
}

bool ringbuf_push(ring_buf_t *rbuf, uint8_t data) {
    size_t next_tail = (rbuf->tail + 1) % rbuf->size;

    if (next_tail != rbuf->head) {
        rbuf->buffer[rbuf->tail] = data;
        rbuf->tail = next_tail;
        return true;
    }

    // full
    return false;
}

bool ringbuf_pop(ring_buf_t *rbuf, uint8_t *data) {
    if (rbuf->head == rbuf->tail) {
        // empty
        return false;
    }

    *data = rbuf->buffer[rbuf->head];
    rbuf->head = (rbuf->head + 1) % rbuf->size;
    return true;
}

bool ringbuf_is_empty(ring_buf_t *rbuf) {
    return rbuf->head == rbuf->tail;
}

bool ringbuf_is_full(ring_buf_t *rbuf) {
    return ((rbuf->tail + 1) % rbuf->size) == rbuf->head;
}

size_t ringbuf_available_data(ring_buf_t *rbuf) {
    return (rbuf->tail - rbuf->head + rbuf->size) % rbuf->size;
}

size_t ringbuf_available_space(ring_buf_t *rbuf) {
    return rbuf->size - ringbuf_available_data(rbuf) - 1;
}
