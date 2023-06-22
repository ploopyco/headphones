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
 * Ben Brewer, a.k.a. flatmush
 * for his exceptional work on libfixmath, on which this is based.
 */

#ifndef FIX16_H
#define FIX16_H

#include <stdbool.h>
#include <inttypes.h>

// During development, it can be useful to run with real double values for reference.
//#define USE_DOUBLE
#ifdef USE_DOUBLE
typedef double fix16_t;
static const fix16_t fix16_zero = 0;
static const fix16_t fix16_one = 1;
#else
// We normalize all values into the range -32..32 with 1 extra bit for overflows
// and one bit for the sign. We allow fixed point values to overflow, but they
// are clipped at the point they are written back to a s16sample.
//
// The reason for normalizing the samples is because the filter coefficients are
// small (usually well within in the range -32..32), by normalizing everything the coefficients
// get lots of additional bits of precision.
typedef int32_t fix16_t;
static const fix16_t fix16_lsb = 0x8000;
static const fix16_t fix16_one = 0x002000000;
static const fix16_t fix16_zero = 0x00000000;
#endif

fix16_t fix16_from_s16sample(int16_t);
int16_t fix16_to_s16sample(fix16_t);
fix16_t fix16_from_dbl(double);
double fix16_to_dbl(fix16_t);

fix16_t fix16_mul(fix16_t, fix16_t);

#endif