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
 */

#include "user.h"
#include "bqf.h"
#include "run.h"

/*****************************************************************************
 * Here is where your digital signal processing journey begins. Follow this
 * guide, and don't forget any steps!
 *
 * 1. Go to user.h and change FILTER_STAGES to the number of filter stages you
 * want.
 * 2. Define the filters that you want to use. Check out "bqf.c" for a
 * complete list of what they are and how they work. Using those filters, you
 * can create ANY digital signal shape you want. Anything you can dream of.
 * 3. You're done! Enjoy the sounds of anything you want.
 ****************************************************************************/

void define_filters() {
    // First filter.
    bqf_memreset(&bqf_filters_mem_left[0]);
    bqf_memreset(&bqf_filters_mem_right[0]);
    bqf_peaking_config(SAMPLING_FREQ, 38.0, -22.0, 0.9, &bqf_filters_left[0]);
    bqf_peaking_config(SAMPLING_FREQ, 38.0, -22.0, 0.9, &bqf_filters_right[0]);

    // Second filter.
    bqf_memreset(&bqf_filters_mem_left[1]);
    bqf_memreset(&bqf_filters_mem_right[1]);
    bqf_peaking_config(SAMPLING_FREQ, 430.0, 6.0, 3.5, &bqf_filters_left[1]);
    bqf_peaking_config(SAMPLING_FREQ, 430.0, 6.0, 3.5, &bqf_filters_right[1]);

    // Third filter.
    bqf_memreset(&bqf_filters_mem_left[2]);
    bqf_memreset(&bqf_filters_mem_right[2]);
    bqf_peaking_config(SAMPLING_FREQ, 2200.0, 7.0, 4.0, &bqf_filters_left[2]);
    bqf_peaking_config(SAMPLING_FREQ, 2200.0, 7.0, 4.0, &bqf_filters_right[2]);

    // Fourth filter.
    bqf_memreset(&bqf_filters_mem_left[3]);
    bqf_memreset(&bqf_filters_mem_right[3]);
    bqf_peaking_config(SAMPLING_FREQ, 3500.0, -5.0, 2.0, &bqf_filters_left[3]);
    bqf_peaking_config(SAMPLING_FREQ, 3500.0, -5.0, 2.0, &bqf_filters_right[3]);

    // Fifth filter.
    bqf_memreset(&bqf_filters_mem_left[4]);
    bqf_memreset(&bqf_filters_mem_right[4]);
    bqf_peaking_config(SAMPLING_FREQ, 6800.0, -6.0, 3.0, &bqf_filters_left[4]);
    bqf_peaking_config(SAMPLING_FREQ, 6800.0, -6.0, 3.0, &bqf_filters_right[4]);

    // Sixth filter.
    bqf_memreset(&bqf_filters_mem_left[5]);
    bqf_memreset(&bqf_filters_mem_right[5]);
    bqf_peaking_config(SAMPLING_FREQ, 9200.0, 3.0, 4.0, &bqf_filters_left[5]);
    bqf_peaking_config(SAMPLING_FREQ, 9200.0, 3.0, 4.0, &bqf_filters_right[5]);
}
