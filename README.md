# :beer: Beer Scale :beer:
This has a small 2.8" TFT display with a capactivie touch screen that can be embedded into a beer tap handle.  Coupled with a digital scale (some load cells with a HX711 controller) to let you know how much beer is left in the keg.

![alt text](https://github.com/cklopotic/BeerScale/blob/main/pictures/Screen%20In%20Tap%20handle.jpg?raw=true)

- SD card contents - Format to either FAT16 or FAT32
    -BMPs to be in 24bit format, can follow templates or provided examples
    -The larger LOGO image is the image used for the main run time display. The Thumb image is for a preview to select when switching logos
    -Match the LOGO and the Thumb Image numbers.  The file names need to stay in provided format.

- If you are having issues with the SD card, try soldering in some 10k pull-up resistors from the CS pins to VDD.
    I had this issue with certain SD cards, it worked for me. https://forums.adafruit.com/viewtopic.php?p=515868

- Check out the docs & pictures sections to see how to integrate the hardware along with some example and reference files to make your own custom flavor.

- On main screen, you can touch (might have to hold for a bit) the scale area to calibration the scale.  Touch and hold the main beer logo to select a different beer (if you have the SD card loaded with different images).
## Sorry, this is a work in Progress...
- Code is a bit of a mess. Needs some refactoring.
- Documentation is crude, dig around you should find what you need
- First version was done with a Raspberry Pi and whatever other parts I had laying around. (https://github.com/cklopotic/WhatsLeft)

## Future state Expansion
- Allow for selection of different keg sizes (currently hard coded for 1/2 barrel)
- More flexibility with scale calibration (currently you need a fixed 120 lbs source to calibrate the high end of the scale)
- Hardware has WiFi and Bluetooth capabilities. Possible future state to encorporate an app.
    -- App could upload different images, calibration flexibiity, adjust custom keg sizes & weights, notifications, remote monitoring, etc.
