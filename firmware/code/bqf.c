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

#include <math.h>
#include <stdio.h>

#include "bqf.h"

int filter_stages = 0;
bqf_coeff_t bqf_filters_left[MAX_FILTER_STAGES];
bqf_coeff_t bqf_filters_right[MAX_FILTER_STAGES];
bqf_mem_t bqf_filters_mem_left[MAX_FILTER_STAGES];
bqf_mem_t bqf_filters_mem_right[MAX_FILTER_STAGES];

/**
 * Configure a low-pass filter. Parameters are as follows:
 *
 * fs: The sampling frequency. This is usually defined for you by
 * SAMPLING_FREQ in run.h. It's the sampling frequency of the DAC on the
 * board.
 *
 * f0: The centre frequency. this is where the signal starts getting
 * attenuated.
 *
 * Q: The quality factor. This is hard to explain. If you want to sound smart,
 * though, just start saying things like "Linkwitz-Riley filters are superior
 * due to their multi-stage flat summations to unity gain". Some example
 * values for this:
 *  - 1/sqrt(2). A "Butterworth" filter. Use this by default; it results in
 *    maximally-flat passband.
 *  - 1/sqrt(3). A "Bessel" filter. Results in maximally-flat group delay.
 *  - 1/2. A "Linkwitz-Riley" filter. Used for sounding smart. Optionally,
 *    used to make lowpass and highpass sections that sum flat to unity gain.
 */
void bqf_lowpass_config(double fs, double f0, double Q, bqf_coeff_t *coefficients) {
    double w0 = 2.0 * M_PI * f0 / fs;

    double cosw0 = cos(w0);
    double sinw0 = sin(w0);
    double alpha = sinw0 / (2.0 * Q);

    double b0 = (1.0 - cosw0) / 2.0;
    double b1 = 1.0 - cosw0;
    double b2 = (1.0 - cosw0) / 2.0;
    double a0 = 1.0 + alpha;
    double a1 = -2.0 * cosw0;
    double a2 = 1.0 - alpha;

    // Normalise all values to a0
    b0 = b0 / a0;
    b1 = b1 / a0;
    b2 = b2 / a0;
    a1 = a1 / a0;
    a2 = a2 / a0;
    a0 = 1.0;

    coefficients->b0 = fix3_28_from_dbl(b0);
    coefficients->b1 = fix3_28_from_dbl(b1);
    coefficients->b2 = fix3_28_from_dbl(b2);
    coefficients->a0 = fix3_28_from_dbl(a0);
    coefficients->a1 = fix3_28_from_dbl(a1);
    coefficients->a2 = fix3_28_from_dbl(a2);
}

/**
 * Configure a high-pass filter. Parameters are as follows:
 *
 * fs: The sampling frequency. This is usually defined for you by
 * SAMPLING_FREQ in run.h. It's the sampling frequency of the DAC on the
 * board.
 *
 * f0: The centre frequency. this is where the signal starts getting
 * attenuated.
 *
 * Q: The quality factor. This is hard to explain. If you want to sound smart,
 * though, just start saying things like "Linkwitz-Riley filters are superior
 * due to their multi-stage flat summations to unity gain". Some example
 * values for this:
 *  - 1/sqrt(2). A "Butterworth" filter. Use this by default; it results in
 *    maximally-flat passband.
 *  - 1/sqrt(3). A "Bessel" filter. Results in maximally-flat group delay.
 *  - 1/2. A "Linkwitz-Riley" filter. Used for sounding smart. Optionally,
 *    used to make lowpass and highpass sections that sum flat to unity gain.
 */
void bqf_highpass_config(double fs, double f0, double Q, bqf_coeff_t *coefficients) {
    double w0 = 2.0 * M_PI * f0 / fs;

    double cosw0 = cos(w0);
    double sinw0 = sin(w0);
    double alpha = sinw0 / (2.0 * Q);

    double b0 = (1.0 + cosw0) / 2.0;
    double b1 = -(1.0 + cosw0);
    double b2 = (1.0 + cosw0) / 2.0;
    double a0 = 1.0 + alpha;
    double a1 = -2.0 * cosw0;
    double a2 = 1.0 - alpha;

    // Normalise all values to a0
    b0 = b0 / a0;
    b1 = b1 / a0;
    b2 = b2 / a0;
    a1 = a1 / a0;
    a2 = a2 / a0;
    a0 = 1.0;

    coefficients->b0 = fix3_28_from_dbl(b0);
    coefficients->b1 = fix3_28_from_dbl(b1);
    coefficients->b2 = fix3_28_from_dbl(b2);
    coefficients->a0 = fix3_28_from_dbl(a0);
    coefficients->a1 = fix3_28_from_dbl(a1);
    coefficients->a2 = fix3_28_from_dbl(a2);
}

/**
 * Configure a band-pass filter, with constant skirt gain - which has a peak
 * gain of Q. Parameters are as follows:
 *
 * fs: The sampling frequency. This is usually defined for you by
 * SAMPLING_FREQ in run.h. It's the sampling frequency of the DAC on the
 * board.
 *
 * f0: The centre frequency. this is where the signal starts getting
 * attenuated.
 *
 * Q: The quality factor. It defines how aggressive the band pass attenuates
 * from the centre frequency. Some example values for Q:
 *  - sqrt(2) is 1 octave wide
 */
void bqf_bandpass_skirt_config(double fs, double f0, double Q, bqf_coeff_t *coefficients) {
    double w0 = 2.0 * M_PI * f0 / fs;

    double cosw0 = cos(w0);
    double sinw0 = sin(w0);
    double alpha = sinw0 / (2.0 * Q);

    double b0 = sinw0 / 2.0;
    double b1 = 0.0;
    double b2 = -sinw0 / 2.0;
    double a0 = 1.0 + alpha;
    double a1 = -2.0 * cosw0;
    double a2 = 1.0 - alpha;

    // Normalise all values to a0
    b0 = b0 / a0;
    b1 = b1 / a0;
    b2 = b2 / a0;
    a1 = a1 / a0;
    a2 = a2 / a0;
    a0 = 1.0;

    coefficients->b0 = fix3_28_from_dbl(b0);
    coefficients->b1 = fix3_28_from_dbl(b1);
    coefficients->b2 = fix3_28_from_dbl(b2);
    coefficients->a0 = fix3_28_from_dbl(a0);
    coefficients->a1 = fix3_28_from_dbl(a1);
    coefficients->a2 = fix3_28_from_dbl(a2);
}

/**
 * Configure a band-pass filter, with constant peak gain of 0 dB. Parameters
 * are as follows:
 *
 * fs: The sampling frequency. This is usually defined for you by
 * SAMPLING_FREQ in run.h. It's the sampling frequency of the DAC on the
 * board.
 *
 * f0: The centre frequency. this is where the signal starts getting
 * attenuated.
 *
 * Q: The quality factor. It defines how aggressive the band pass attenuates
 * from the centre frequency. Some example values for Q:
 *  - sqrt(2) is 1 octave wide
 */
void bqf_bandpass_peak_config(double fs, double f0, double Q, bqf_coeff_t *coefficients) {
    double w0 = 2.0 * M_PI * f0 / fs;

    double cosw0 = cos(w0);
    double sinw0 = sin(w0);
    double alpha = sinw0 / (2.0 * Q);

    double b0 = alpha;
    double b1 = 0.0;
    double b2 = -alpha;
    double a0 = 1.0 + alpha;
    double a1 = -2.0 * cosw0;
    double a2 = 1.0 - alpha;

    // Normalise all values to a0
    b0 = b0 / a0;
    b1 = b1 / a0;
    b2 = b2 / a0;
    a1 = a1 / a0;
    a2 = a2 / a0;
    a0 = 1.0;

    coefficients->b0 = fix3_28_from_dbl(b0);
    coefficients->b1 = fix3_28_from_dbl(b1);
    coefficients->b2 = fix3_28_from_dbl(b2);
    coefficients->a0 = fix3_28_from_dbl(a0);
    coefficients->a1 = fix3_28_from_dbl(a1);
    coefficients->a2 = fix3_28_from_dbl(a2);
}

/**
 * Configure a notch filter. Parameters are as follows:
 *
 * fs: The sampling frequency. This is usually defined for you by
 * SAMPLING_FREQ in run.h. It's the sampling frequency of the DAC on the
 * board.
 *
 * f0: The centre frequency. this is where the signal starts getting
 * attenuated.
 *
 * Q: The quality factor. It defines how aggressive the notch attenuates
 * from the centre frequency. Some example values for Q:
 *  - sqrt(2) is 1 octave wide
 */
void bqf_notch_config(double fs, double f0, double Q, bqf_coeff_t *coefficients) {
    double w0 = 2.0 * M_PI * f0 / fs;

    double cosw0 = cos(w0);
    double sinw0 = sin(w0);
    double alpha = sinw0 / (2.0 * Q);

    double b0 = 1.0;
    double b1 = -2.0 * cosw0;
    double b2 = 1.0;
    double a0 = 1.0 + alpha;
    double a1 = -2.0 * cosw0;
    double a2 = 1.0 - alpha;

    // Normalise all values to a0
    b0 = b0 / a0;
    b1 = b1 / a0;
    b2 = b2 / a0;
    a1 = a1 / a0;
    a2 = a2 / a0;
    a0 = 1.0;

    coefficients->b0 = fix3_28_from_dbl(b0);
    coefficients->b1 = fix3_28_from_dbl(b1);
    coefficients->b2 = fix3_28_from_dbl(b2);
    coefficients->a0 = fix3_28_from_dbl(a0);
    coefficients->a1 = fix3_28_from_dbl(a1);
    coefficients->a2 = fix3_28_from_dbl(a2);
}

/**
 * Configure an allpass filter. Parameters are as follows:
 *
 * fs: The sampling frequency. This is usually defined for you by
 * SAMPLING_FREQ in run.h. It's the sampling frequency of the DAC on the
 * board.
 *
 * f0: The centre frequency. this is where the signal starts getting
 * attenuated.
 *
 * Q: The quality factor. I don't actually know what this is for an allpass.
 * Try experimenting. Why do I have to do all the work?
 */
void bqf_allpass_config(double fs, double f0, double Q, bqf_coeff_t *coefficients) {
    double w0 = 2.0 * M_PI * f0 / fs;

    double cosw0 = cos(w0);
    double sinw0 = sin(w0);
    double alpha = sinw0 / (2.0 * Q);

    double b0 = 1.0 - alpha;
    double b1 = -2.0 * cosw0;
    double b2 = 1.0 + alpha;
    double a0 = 1.0 + alpha;
    double a1 = -2.0 * cosw0;
    double a2 = 1.0 - alpha;

    // Normalise all values to a0
    b0 = b0 / a0;
    b1 = b1 / a0;
    b2 = b2 / a0;
    a1 = a1 / a0;
    a2 = a2 / a0;
    a0 = 1.0;

    coefficients->b0 = fix3_28_from_dbl(b0);
    coefficients->b1 = fix3_28_from_dbl(b1);
    coefficients->b2 = fix3_28_from_dbl(b2);
    coefficients->a0 = fix3_28_from_dbl(a0);
    coefficients->a1 = fix3_28_from_dbl(a1);
    coefficients->a2 = fix3_28_from_dbl(a2);
}

/**
 * Configure a peaking filter. Parameters are as follows:
 *
 * fs: The sampling frequency. This is usually defined for you by
 * SAMPLING_FREQ in run.h. It's the sampling frequency of the DAC on the
 * board.
 *
 * f0: The centre frequency. this is where the signal starts getting
 * attenuated.
 *
 * dBgain: The gain at the centre frequency, in dB. Positive for boost,
 * negative for cut.
 *
 * Q: The quality factor. It defines the bandwidth from the centre frequency.
 * For the purposes of this filter, the bandwidth is defined using the points
 * on the curve at which the gain in dB is half of the peak gain. Some
 * example values for Q:
 *  - sqrt(2) is 1 octave wide
 */
void bqf_peaking_config(double fs, double f0, double dBgain, double Q,
        bqf_coeff_t *coefficients) {

    double A = pow(10.0, (dBgain/40));

    double w0 = 2.0 * M_PI * f0 / fs;

    double cosw0 = cos(w0);
    double sinw0 = sin(w0);
    double alpha = sinw0 / (2.0 * Q);

    double b0 = 1.0 + (alpha * A);
    double b1 = -2.0 * cosw0;
    double b2 = 1.0 - (alpha * A);
    double a0 = 1.0 + (alpha / A);
    double a1 = -2.0 * cosw0;
    double a2 = 1.0 - (alpha / A);

    // Normalise all values to a0
    b0 = b0 / a0;
    b1 = b1 / a0;
    b2 = b2 / a0;
    a1 = a1 / a0;
    a2 = a2 / a0;
    a0 = 1.0;

    coefficients->b0 = fix3_28_from_dbl(b0);
    coefficients->b1 = fix3_28_from_dbl(b1);
    coefficients->b2 = fix3_28_from_dbl(b2);
    coefficients->a0 = fix3_28_from_dbl(a0);
    coefficients->a1 = fix3_28_from_dbl(a1);
    coefficients->a2 = fix3_28_from_dbl(a2);
}

/**
 * Configure a low-shelf filter. Parameters are as follows:
 *
 * fs: The sampling frequency. This is usually defined for you by
 * SAMPLING_FREQ in run.h. It's the sampling frequency of the DAC on the
 * board.
 *
 * f0: The centre frequency. this is where the signal starts getting
 * attenuated.
 *
 * dBgain: The gain at the centre frequency, in dB. Positive for boost,
 * negative for cut.
 *
 * Q: The quality factor. It defines the steepness of the shelf's slope. I
 * don't actually know what this is for a shelf filter. Try experimenting.
 * Why do I have to do all the work?
 */
void bqf_lowshelf_config(double fs, double f0, double dBgain, double Q,
        bqf_coeff_t *coefficients) {

    double A = pow(10.0, (dBgain/40));

    double w0 = 2.0 * M_PI * f0 / fs;

    double cosw0 = cos(w0);
    double sinw0 = sin(w0);
    double alpha = sinw0 / (2.0 * Q);

    double trAa = 2 * sqrt(A) * alpha;

    double b0 = A * ((A + 1) - ((A - 1) * cosw0) + trAa);
    double b1 = 2 * A * ((A - 1) - ((A + 1) * cosw0));
    double b2 = A * ((A + 1) - ((A - 1) * cosw0) - trAa);
    double a0 = (A + 1) + ((A - 1) * cosw0) + trAa;
    double a1 = -2 * ((A - 1) + ((A + 1) * cosw0));
    double a2 = (A + 1) + ((A - 1) * cosw0) - trAa;

    // Normalise all values to a0
    b0 = b0 / a0;
    b1 = b1 / a0;
    b2 = b2 / a0;
    a1 = a1 / a0;
    a2 = a2 / a0;
    a0 = 1.0;

    coefficients->b0 = fix3_28_from_dbl(b0);
    coefficients->b1 = fix3_28_from_dbl(b1);
    coefficients->b2 = fix3_28_from_dbl(b2);
    coefficients->a0 = fix3_28_from_dbl(a0);
    coefficients->a1 = fix3_28_from_dbl(a1);
    coefficients->a2 = fix3_28_from_dbl(a2);
}

/**
 * Configure a high-shelf filter. Parameters are as follows:
 *
 * fs: The sampling frequency. This is usually defined for you by
 * SAMPLING_FREQ in run.h. It's the sampling frequency of the DAC on the
 * board.
 *
 * f0: The centre frequency. this is where the signal starts getting
 * attenuated.
 *
 * dBgain: The gain at the centre frequency, in dB. Positive for boost,
 * negative for cut.
 *
 * Q: The quality factor. It defines the steepness of the shelf's slope. I
 * don't actually know what this is for a shelf filter. Try experimenting.
 * Why do I have to do all the work?
 */
void bqf_highshelf_config(double fs, double f0, double dBgain, double Q,
        bqf_coeff_t *coefficients) {

    double A = pow(10.0, (dBgain/40));

    double w0 = 2.0 * M_PI * f0 / fs;

    double cosw0 = cos(w0);
    double sinw0 = sin(w0);
    double alpha = sinw0 / (2.0 * Q);

    double trAa = 2 * sqrt(A) * alpha;

    double b0 = A * ((A + 1) + ((A - 1) * cosw0) + trAa);
    double b1 = -2 * A * ((A - 1) + ((A + 1) * cosw0));
    double b2 = A * ((A + 1) + ((A - 1) * cosw0) - trAa);
    double a0 = (A + 1) - ((A - 1) * cosw0) + trAa;
    double a1 = 2 * ((A - 1) - ((A + 1) * cosw0));
    double a2 = (A + 1) - ((A - 1) * cosw0) - trAa;

    // Normalise all values to a0
    b0 = b0 / a0;
    b1 = b1 / a0;
    b2 = b2 / a0;
    a1 = a1 / a0;
    a2 = a2 / a0;
    a0 = 1.0;

    coefficients->b0 = fix3_28_from_dbl(b0);
    coefficients->b1 = fix3_28_from_dbl(b1);
    coefficients->b2 = fix3_28_from_dbl(b2);
    coefficients->a0 = fix3_28_from_dbl(a0);
    coefficients->a1 = fix3_28_from_dbl(a1);
    coefficients->a2 = fix3_28_from_dbl(a2);
}

void bqf_memreset(bqf_mem_t *memory) {
    memory->x_1 = fix16_zero;
    memory->x_2 = fix16_zero;
    memory->y_1 = fix16_zero;
    memory->y_2 = fix16_zero;
}
