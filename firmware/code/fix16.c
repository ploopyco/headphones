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

#include "fix16.h"

fix16_t fix16_from_int(int16_t a) {
    return a * fix16_one;
}

int16_t fix16_to_int(fix16_t a) {
    if (a >= 0)
        return (a + (fix16_one >> 1)) / fix16_one;
    return (a - (fix16_one >> 1)) / fix16_one;
}

fix16_t fix16_from_dbl(double a) {
    double temp = a * fix16_one;
    temp += (double)((temp >= 0) ? 0.5f : -0.5f);
    return (fix16_t)temp;
}

double fix16_to_dbl(fix16_t a) {
    return (double)a / fix16_one;
}

// hic sunt dracones
fix16_t fix16_mul(fix16_t inArg0, fix16_t inArg1) {
    int64_t product = (int64_t)inArg0 * inArg1;

    uint32_t upper = (product >> 47);
    if (product < 0) {
        if (~upper)
            return fix16_overflow;
        product--;
    } else {
        if (upper)
            return fix16_overflow;
    }

    fix16_t result = product >> 15;
    result += (product & 0x4000) >> 14;

    return result;
}
