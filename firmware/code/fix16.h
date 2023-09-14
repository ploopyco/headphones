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


/// @brief Fixed point math type, in format Q3.28. One sign bit, 3 bits for left-of-decimal
///and 28 for right-of-decimal. This arrangment works because we normalize the incoming USB
///audio data to ]-1,1[ before operating on it, to push as much of the precision in the signal
///to the right of the decimal as possible.
typedef int32_t fix3_28_t;

/// @brief Represents the number 1 in Q3.28. There are 3 bits and a sign bit to the left of the
///decimal, so 1.0000 would be represented as 0b 0001 followed by 28 zeros.
static const fix3_28_t fix16_one =    0x10000000;

/// @brief Represents zero in fixed point world.
static const fix3_28_t fix16_zero = 0x00000000;

static inline fix3_28_t norm_fix3_28_from_s16sample(int16_t);

static inline int32_t norm_fix3_28_to_s16sample(fix3_28_t);

static inline fix3_28_t fix3_28_from_flt(float);

static inline fix3_28_t fix3_28_from_dbl(double);

static inline fix3_28_t fix16_mul(fix3_28_t, fix3_28_t);

#include "fix16.inl"
#endif