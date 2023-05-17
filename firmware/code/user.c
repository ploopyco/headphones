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

int filter_stages = 0;

/*****************************************************************************
 * Here is where your digital signal processing journey begins. Follow this
 * guide, and don't forget any steps!
 *
 * 1. Define the filters that you want to use. Check out "bqf.c" for a
 * complete list of what they are and how they work. Using those filters, you
 * can create ANY digital signal shape you want. Anything you can dream of.
 * 2. You're done! Enjoy the sounds of anything you want.
 ****************************************************************************/

void define_filters() {
    // First filter.
    bqf_memreset(&bqf_filters_mem_left[filter_stages]);
    bqf_memreset(&bqf_filters_mem_right[filter_stages]);
    bqf_peaking_config(SAMPLING_FREQ, 38.0, -19.0, 0.9, &bqf_filters_left[filter_stages]);
    bqf_peaking_config(SAMPLING_FREQ, 38.0, -19.0, 0.9, &bqf_filters_right[filter_stages++]);
    
    // Second filter.
    bqf_memreset(&bqf_filters_mem_left[filter_stages]);
    bqf_memreset(&bqf_filters_mem_right[filter_stages]);
    bqf_lowshelf_config(SAMPLING_FREQ, 2900.0, 3.0, 4.0, &bqf_filters_left[filter_stages]);
    bqf_lowshelf_config(SAMPLING_FREQ, 2900.0, 3.0, 4.0, &bqf_filters_right[filter_stages++]);

    // Third filter.
    bqf_memreset(&bqf_filters_mem_left[filter_stages]);
    bqf_memreset(&bqf_filters_mem_right[filter_stages]);
    bqf_peaking_config(SAMPLING_FREQ, 430.0, 6.0, 3.5, &bqf_filters_left[filter_stages]);
    bqf_peaking_config(SAMPLING_FREQ, 430.0, 6.0, 3.5, &bqf_filters_right[filter_stages++]);

    // Fourth filter.
    bqf_memreset(&bqf_filters_mem_left[filter_stages]);
    bqf_memreset(&bqf_filters_mem_right[filter_stages]);
    bqf_highshelf_config(SAMPLING_FREQ, 8400.0, 3.0, 4.0, &bqf_filters_left[filter_stages]);
    bqf_highshelf_config(SAMPLING_FREQ, 8400.0, 3.0, 4.0, &bqf_filters_right[filter_stages++]);

    // Fifth filter.
    bqf_memreset(&bqf_filters_mem_left[filter_stages]);
    bqf_memreset(&bqf_filters_mem_right[filter_stages]);
    bqf_peaking_config(SAMPLING_FREQ, 4800.0, 6.0, 5.0, &bqf_filters_left[filter_stages]);
    bqf_peaking_config(SAMPLING_FREQ, 4800.0, 6.0, 5.0, &bqf_filters_right[filter_stages++]);
}
