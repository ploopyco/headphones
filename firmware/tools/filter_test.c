#include <stdio.h>
#include <stdlib.h>
#include "bqf.h"
#include "fix16.h"
#include "user.h"

bqf_coeff_t bqf_filters_left[MAX_FILTER_STAGES];
bqf_coeff_t bqf_filters_right[MAX_FILTER_STAGES];
bqf_mem_t bqf_filters_mem_left[MAX_FILTER_STAGES];
bqf_mem_t bqf_filters_mem_right[MAX_FILTER_STAGES];

const char* usage = "Usage: %s INFILE OUTFILE\n\n"
    "Reads 16bit stereo PCM data from INFILE, runs it through the Ploopy headphones\n"
    "filters then writes it out to OUTFILE.\n";

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        fprintf(stdout, usage, argv[0]);
        exit(1);
    }

    // Load the input data into a buffer
    FILE* input = fopen(argv[1], "rb");
    if (!input) {
        fprintf(stderr, "Cannot open input file '%s'\n", argv[1]);
        exit(1);
    }

    // Get the file size
    fseek(input , 0L , SEEK_END);
    size_t input_size = ftell(input);
    rewind(input);

    // Allocate our input and output buffers. This could be optimized
    // we dont need to store the whole input and output files in memory.
    int samples = input_size / 2;
    int16_t *in = (int16_t *) calloc(samples, sizeof(int16_t));
    int16_t *out = (int16_t *) calloc(samples, sizeof(int16_t));

    fread(in, samples, sizeof(int16_t), input);
    fclose(input);

    // Open the output file.
    FILE* output = fopen(argv[2], "wb");
    if (!output)
    {
        fprintf(stderr, "Cannot open output file '%s'\n", argv[2]);
        exit(1);
    }

    // The smaple proccesing code, essentially the same as the
    // code in the firmware's run.c file.
    define_filters();

    for (int i = 0; i < samples; i++)
    {
        out[i] = in[i];
    }

    for (int j = 0; j < FILTER_STAGES; j++)
    {
        for (int i = 0; i < samples; i ++)
        {
            // Left channel filter
            fix16_t x_f16 = fix16_from_int((int16_t) out[i]);

            x_f16 = bqf_transform(x_f16, &bqf_filters_left[j],
                &bqf_filters_mem_left[j]);

            out[i] = (int32_t) fix16_to_int(x_f16);

            // Right channel filter
            i++;
            x_f16 = fix16_from_int((int16_t) out[i]);

            x_f16 = bqf_transform(x_f16, &bqf_filters_right[j],
                &bqf_filters_mem_right[j]);

            out[i] = (int16_t) fix16_to_int(x_f16);
        }
    }

    // Write out the processed audio.
    fwrite(out, samples, sizeof(int16_t), output);
    fclose(output);

    free(in);
    free(out);
}
