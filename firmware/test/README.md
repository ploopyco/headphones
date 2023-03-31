## filter_test
This is a basic utility for testing the Ploopy headphones filtering routines on a PC.

### Usage
Find a source file and use ffmpeg to convert it to PCM data:

```
ffmpeg -i <input file< -map 0:6 -vn -f s16le -acodec pcm_s16le input.pcm
```

Run `filter_test` to process the PCM data. The `filter_test` program takes two arguments an input file and an output file:

```
./filter_test input.pcm output.pcm
```

You can listen to the output audio using ffplay (which is usually included with ffmpeg:

```
./ffplay -f s16le -ar 48000 -ac 2 output.pcm
```

If there are no obvious problems, go ahead and flash your firmware.