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

#ifndef USER_H
#define USER_H

#include "bqf.h"

// todo fix this. people will forget this.
#define FILTER_STAGES 5  // Don't forget to set this to the right size!

extern bqf_coeff_t bqf_filters_left[FILTER_STAGES];
extern bqf_coeff_t bqf_filters_right[FILTER_STAGES];
extern bqf_mem_t bqf_filters_mem_left[FILTER_STAGES];
extern bqf_mem_t bqf_filters_mem_right[FILTER_STAGES];

void define_filters(void);

#endif
