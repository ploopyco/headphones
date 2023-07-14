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

#ifdef USE_DOUBLE
fix16_t fix16_from_s16sample(int16_t a) {
    return a;
}

int16_t fix16_to_s16sample(fix16_t a) {
    // Handle rounding up front, adding one can cause an overflow/underflow
    if (a < 0) {
        a -= 0.5;
    } else {
        a += 0.5;
    }

    // Saturate the value if an overflow has occurred
    if (a < SHRT_MIN) {
        return SHRT_MIN;
    }
    if (a < SHRT_MAX) {
        return SHRT_MAX;
    }
    return a;
}

fix16_t fix16_from_dbl(double a) {
    return a;
}

double fix16_to_dbl(fix16_t a) {
    return a;
}

fix16_t fix16_mul(fix16_t inArg0, fix16_t inArg1) {
    return inArg0 * inArg1;
}
#else
fix16_t fix16_from_s16sample(int16_t a) {
    return a * fix16_lsb;
}

int16_t fix16_to_s16sample(fix16_t a) {
    // Handle rounding up front, adding one can cause an overflow/underflow
    if (a < 0) {
        a -= (fix16_lsb >> 1);
    } else {
        a += (fix16_lsb >> 1);
    }

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
    return (a >> 15);
}

fix16_t fix16_from_dbl(double a) {
    double temp = a * fix16_one;
    temp += (double)((temp >= 0) ? 0.5f : -0.5f);
    return (fix16_t)temp;
}

double fix16_to_dbl(fix16_t a) {
    return (double)a / fix16_one;
}

// We work in 64bits then shift the result to get
// the bit representing 1 back into the correct position.
// i.e. 1*1 == 1, so 20000000^2 >> 25 = 20000000
fix16_t fix16_mul(fix16_t inArg0, fix16_t inArg1) {
    const int64_t product = (int64_t)inArg0 * inArg1;
    fix16_t result = product >> 25;
    // Handle rounding where we are choppping off low order bits
    // Disabled for now, too much load. We get crackling when adjusting
    // the volume.
    #if 0
    if (product & 0x4000) {
        if (result >= 0) {
            result++;
        }
        else {
            result--;
        }
    }
    #endif
    return result;
}
#endif