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
 * Robert Bristow-Johnson, a.k.a. RBJ
 * for his exceptional work on biquad formulae as applied to digital
 * audio filtering, summarised in his pamphlet, "Audio EQ Cookbook".
 */

static inline fix3_28_t bqf_transform(fix3_28_t x, bqf_coeff_t *coefficients, bqf_mem_t *memory) {
    fix3_28_t y = fix16_mul(coefficients->b0, x) -
            fix16_mul(coefficients->a1, memory->y_1) +
            fix16_mul(coefficients->b1, memory->x_1) -
            fix16_mul(coefficients->a2, memory->y_2) +
            fix16_mul(coefficients->b2, memory->x_2);

    memory->x_2 = memory->x_1;
    memory->x_1 = x;
    memory->y_2 = memory->y_1;
    memory->y_1 = y;

    return y;
}