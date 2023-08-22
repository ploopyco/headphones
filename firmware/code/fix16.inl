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

#include <stdio.h>
#include <inttypes.h>
#include <math.h>
#include <limits.h>
#include "fix16.h"

/// @brief Produces a fixed point number from a 16-bit signed integer, normalized to ]-1,1[.
/// @param a Signed 16-bit integer.
/// @return A fixed point number in Q3.28 format, with input normalized to ]-1,1[.
static inline fix3_28_t norm_fix3_28_from_s16sample(int16_t a) {
    /* So, we're using a Q3.28 fixed point system here, and we want the incoming
       audio signal to be represented as a number between -1 and 1. To do this,
       we need the 16-bit value to map to the 28-bit right-of-decimal field in
       our fixed point number. 28-16 = 12, so we shift the incoming value by
       that much to covert it to the desired Q3.28 format and do the normalization
       all in one go.
    */
    return (fix3_28_t)a << 12;
}

/// @brief Convert fixed point samples into signed integer. Used to convert
///        calculated sample to one that the DAC can understand.
/// @param a
/// @return Signed 16-bit integer.
static inline int16_t norm_fix3_28_to_s16sample(fix3_28_t a) {
    // Handle rounding up front, adding one can cause an overflow/underflow

    // It's not clear exactly how this works, so we'll disable it for now.
    /*
    if (a < 0) {
        a -= (fix16_lsb >> 1);
    } else {
        a += (fix16_lsb >> 1);
    }
    */

    // Saturate the value if an overflow has occurred
    uint32_t upper = (a >> 30);
    if (a < 0) {
        if (~upper)
        {
            return SHRT_MIN;
        }
    } else {
        if (upper)
        {
            return SHRT_MAX;
        }
    }
    /* When we converted the USB audio sample to a fixed point number, we applied
       a normalization, or a gain of 1/65536. To convert it back, we can undo that
       by shifting it back by the same amount we shifted it in the first place. */
    return (a >> 12);
}

static inline fix3_28_t fix3_28_from_flt(float a) {
    float temp = a * fix16_one;
    temp += ((temp >= 0) ? 0.5f : -0.5f);
    return (fix3_28_t)temp;
}

static inline fix3_28_t fix3_28_from_dbl(double a) {
    double temp = a * fix16_one;
    temp += (double)((temp >= 0) ? 0.5f : -0.5f);
    return (fix3_28_t)temp;
}

/// @brief Multiplies two fixed point numbers in Q3.28 format together.
/// @param inArg0 Q3.28 format fixed point number.
/// @param inArg1 Q3.28 format fixed point number.
/// @return A Q3.28 fixed point number that represents the truncated result of inArg0 x inArg1.
static inline fix3_28_t fix16_mul(fix3_28_t inArg0, fix3_28_t inArg1) {
    int32_t A = (inArg0 >> 14), C = (inArg1 >> 14);
    uint32_t B = (inArg0 & 0x3FFF), D = (inArg1 & 0x3FFF);
    int32_t AC = A*C;
    int32_t AD_CB = A*D + C*B;
    int32_t product_hi = AC + (AD_CB >> 14);

#if HANDLE_CARRY
	// Handle carry from lower bits to upper part of result.
    uint32_t BD = B*D;
	uint32_t ad_cb_temp = AD_CB << 14;
	uint32_t product_lo = BD + ad_cb_temp;

	if (product_lo < BD)
		product_hi++;
#endif

    return product_hi;
}