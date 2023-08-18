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

#ifndef RINGBUF_H
#define RINGBUF_H

#include "pico/stdlib.h"

typedef struct _ring_buf_t {
    uint32_t *buffer;
    size_t head;
    size_t tail;
    size_t size;
} ring_buf_t;

void ringbuf_init(ring_buf_t *, uint32_t *, size_t);
bool ringbuf_push(ring_buf_t *, uint32_t );
bool ringbuf_pop(ring_buf_t *, uint32_t *);
bool ringbuf_is_empty(ring_buf_t *);
bool ringbuf_is_full(ring_buf_t *);
size_t ringbuf_available_data(ring_buf_t *);
size_t ringbuf_available_space(ring_buf_t *);

#endif