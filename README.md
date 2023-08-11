# WhatsLeftV2
Touch Screen that can be embedded into a beer tap handle, coupled with a scale to let you know how much beer is left in the keg.

- SD card contents - Format to either FAT16 or FAT32
    -BMPs to be in 24bit format, can follow templates or provided examples
    -The larger LOGO image is the image used for the main run time display. The Thumb image is for a preview to select when switching logos
    -Match the LOGO and the Thumb Image numbers.  The file names need to stay in provided format.

- If you are having issues with the SD card, try soldering in some 10k pull-up resistors from the CS pins to VDD.
    I had this issue with certain SD cards, it worked for me. https://forums.adafruit.com/viewtopic.php?p=515868

- Check out the docs & pictures sections to see how to integrate the hardware along with some example and reference files to make your own custom flavor.
