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

#ifndef BQF_H
#define BQF_H

#include "fix16.h"

typedef struct _bqf_coeff_t {
    fix3_28_t a0;
    fix3_28_t a1;
    fix3_28_t a2;
    fix3_28_t b0;
    fix3_28_t b1;
    fix3_28_t b2;
} bqf_coeff_t;

typedef struct _bqf_mem_t {
    fix3_28_t x_1;
    fix3_28_t x_2;
    fix3_28_t y_1;
    fix3_28_t y_2;
} bqf_mem_t;

// More filters should be possible, but the config structure
// might grow beyond the current 512 byte size.
#define MAX_FILTER_STAGES 20
extern int filter_stages;

extern bqf_coeff_t bqf_filters_left[MAX_FILTER_STAGES];
extern bqf_coeff_t bqf_filters_right[MAX_FILTER_STAGES];
extern bqf_mem_t bqf_filters_mem_left[MAX_FILTER_STAGES];
extern bqf_mem_t bqf_filters_mem_right[MAX_FILTER_STAGES];

#define Q_BUTTERWORTH 0.707106781
#define Q_BESSEL 0.577350269
#define Q_LINKWITZ_RILEY 0.5

void bqf_lowpass_config(double, double, double, bqf_coeff_t *);
void bqf_highpass_config(double, double, double, bqf_coeff_t *);
void bqf_bandpass_skirt_config(double, double, double, bqf_coeff_t *);
void bqf_bandpass_peak_config(double, double, double, bqf_coeff_t *);
void bqf_notch_config(double, double, double, bqf_coeff_t *);
void bqf_allpass_config(double, double, double, bqf_coeff_t *);
void bqf_peaking_config(double, double, double, double, bqf_coeff_t *);
void bqf_lowshelf_config(double, double, double, double, bqf_coeff_t *);
void bqf_highshelf_config(double, double, double, double, bqf_coeff_t *);

static inline fix3_28_t bqf_transform(fix3_28_t, bqf_coeff_t *, bqf_mem_t *);
void bqf_memreset(bqf_mem_t *);

#include "bqf.inl"
#endif
