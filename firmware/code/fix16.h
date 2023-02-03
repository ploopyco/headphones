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

#include <inttypes.h>

typedef int32_t fix16_t;

static const fix16_t fix16_overflow = 0x80000000;
static const fix16_t fix16_one = 0x00008000;

fix16_t fix16_from_int(int16_t);
int16_t fix16_to_int(fix16_t);
fix16_t fix16_from_dbl(double);
double fix16_to_dbl(fix16_t);

fix16_t fix16_mul(fix16_t, fix16_t);

#endif