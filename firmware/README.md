## There are three different firmwares available.

Do you see that button on this Github page that says "master"? That allows you to select branches. Different branches have different firmwares.

- The "master" branch has the default firmware.
- The "ploopyco/left-right-reverse" firmware has firmware that flips the left and right channels.
- The "ploopyco/distortion-cut" firmware has firmware that experiences less digital distortion, though it is less loud overall.

## BEFORE YOU FLASH PM8:

pm8 includes significant changes from pm7. It is quite likely that you'll need to make your computer "forget" or "uninstall" the USB entry for the Ploopy Headphones before pm8 works correctly. What does "not working correctly" sound like? Volume adjustment will probably not work at all, the left and right channels may play independently of one another, and there won't be individual volume controls available for the left and right channel, amongst other things.

If you're on Windows:
- Open your Device Manager.
- Find "Universal Serial Bus controllers".
- Do you see a bunch of "USB Composite Device"s? Double-click on the first one to open a "USB Composite Device Properties" window.
- Go to the "Details" tab.
- Under "Property", find "Hardware Ids".
- The "Value" should include the following: `USB\VID_2E8A&PID_FEDD`. Note the *VID* is **2E8A** and the *PID* is **FEDD**.
- If the Value contains different values for the VID and PID, try the next USB Composite Device.
- Once you've found the right USB Composite Device, go back to the Device Manager window, right-click on the right USB Composite Device, and "Uninstall device".
- Wait a few seconds for your Device Manager to update.
- Unplug the Ploopy Headphones from your computer. Wait a few more seconds.
- Plug them back into your computer.
- If everything was done correctly, the new firmware should work as a charm.

If you're on a different OS, like Linux or Mac, you will have to follow different instructions. I don't have Linux or Mac to test this on, unfortunately. If you're having trouble, email contact@ploopy.co.

## Borked your firmware?

All Ploopy Headphones pre-amp boards ship with "ploopy_headphones.uf2". If things aren't working the way you want them to, try reflashing the uf2 file included in this directory.
