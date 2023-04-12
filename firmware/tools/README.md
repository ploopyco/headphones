## filter_test
This is a basic utility for testing the Ploopy headphones filtering routines on a PC.

### Usage
Find a source file and use ffmpeg to convert it to 16bit stereo PCM samples:

```
ffmpeg -i <input file> -map 0:6 -vn -f s16le -acodec pcm_s16le input.pcm
```

Run `filter_test` to process the PCM samples. The `filter_test` program takes two arguments an input file and an output file:

```
./filter_test input.pcm output.pcm
```

You can listen to the PCM files using ffplay (which is usually included with ffmpeg):

```
ffplay -f s16le -ar 48000 -ac 2 output.pcm
```

If there are no obvious problems, go ahead and flash your firmware.

## reboot_bootloader.py
If your Ploopy Headphones firmware is new enough, it has support for a USB vendor command that will cause the RP2040 to reboot into the
bootloader. This will enable you to update the firmware without having to remove the case and short the pins on the board.

### Usage
Connect the Ploopy headphones DAC and run:

```
./reboot_bootloader.py
```

You will need python3 and the pyusb module. If you get a permission denied error you may need to configure your udev rules, or run the
script with administrator privileges.