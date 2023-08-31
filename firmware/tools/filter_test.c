#include <stdio.h>
#include <stdlib.h>
#include "bqf.h"
#include "fix16.h"
#include "configuration_manager.h"

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
    int32_t *out = (int32_t *) calloc(samples, sizeof(int32_t));

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
    load_config();

    for (int i = 0; i < samples; i++)
    {
        out[i] = in[i];
    }

    for (int i = 0; i < samples; i ++)
    {
        // Left channel filter
        fix3_28_t x_f16 = norm_fix3_28_from_s16sample((int16_t) out[i]);

        for (int j = 0; j < filter_stages; j++)
        {
            x_f16 = bqf_transform(x_f16, &bqf_filters_left[j],
                &bqf_filters_mem_left[j]);
        }

        out[i] = (int32_t) norm_fix3_28_to_s16sample(x_f16);

        // Right channel filter
        i++;
        x_f16 = norm_fix3_28_from_s16sample((int16_t) out[i]);

        for (int j = 0; j < filter_stages; j++)
        {
            x_f16 = bqf_transform(x_f16, &bqf_filters_right[j],
                &bqf_filters_mem_right[j]);
        }

        out[i] = (int32_t) norm_fix3_28_to_s16sample(x_f16);
        //printf("%08x\n", out[i]);
    }

    // Write out the processed audio.
    for (int i=0; i<samples; i++) {
        fwrite(&out[i], 3, sizeof(int8_t), output);
    }
    fclose(output);

    free(in);
    free(out);
}
