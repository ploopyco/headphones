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
    fix16_t a0;
    fix16_t a1;
    fix16_t a2;
    fix16_t b0;
    fix16_t b1;
    fix16_t b2;
} bqf_coeff_t;

typedef struct _bqf_mem_t {
    fix16_t x_1;
    fix16_t x_2;
    fix16_t y_1;
    fix16_t y_2;
} bqf_mem_t;

// In reality we do not have enough CPU resource to run 8 filtering
// stages without some optimisation.
#define MAX_FILTER_STAGES 8
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

fix16_t bqf_transform(fix16_t, bqf_coeff_t *, bqf_mem_t *);
void bqf_memreset(bqf_mem_t *);

#endif
