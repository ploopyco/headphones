#!/usr/bin/python3
import usb.core
from usb.util import *

REBOOT_BOOTLOADER = 0
PLOOPY_VID = 0x2e8a
PLOOPY_PID = 0xfedd

device_count = 0

# Find all connected Ploopy headphone devices
for dev in usb.core.find(find_all=True, idVendor=PLOOPY_VID, idProduct=PLOOPY_PID):
    device_count += 1

    # The vendor command expects the wValue to be equal to the Ploopy vendor id. This
    # will hopefully prevent badly behaved software from accidentally triggering bootloader
    # mode.
    try:
        dev.ctrl_transfer(CTRL_RECIPIENT_DEVICE | CTRL_TYPE_VENDOR, REBOOT_BOOTLOADER, PLOOPY_VID)
    except Exception as e:
        # The headphones do not respond to the vendor command, as they have already rebooted,
        # so for now, we always end up here.
        if e.errno == 32:
            # libusb pipe error, this is expected. OpenUSB doesnt report an error.
            pass
        else:
            print(e)

print(f"Sent a reboot command to {device_count} Ploopy devices.")