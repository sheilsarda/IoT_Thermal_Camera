README:
In order to check OTAFU via our dashboard, you can find the correct version numbers and CRC32 values by visiting
https://www.seas.upenn.edu/~sheils/metadata.txt

Line 1 of the metadata file is the version number, line 2 is the CRC32.
You will need to input these values to the Security Tab of our dashboard in order to successfuly update the Application Code.

The ApplicationCode.bin file being downloaded by OTAFU is hosted at 
https://www.seas.upenn.edu/~sheils/ApplicationCode.bin

You can find our dashboard at
https://radiancet.mybluemix.net/ui/#!/0

Since we did not make any modifications to our bootloader from A6, we did not need to add the bootloader project to our Atmel Studio solution for A7. The packaged code submitted in this directory is our application code for A7 only.

Our A6 Bootloader code can be found in the A6 folder in our project directory.

For the purposes of adding modules that we will be using in our final project, we added the Sercom I2C slave and master drivers, in addition to the Sercom SPI serial driver.

For testing purposes, we have also attateched the pre-OTAFU bin files and CRC files, which our bootloader expects to be loaded on the SD Card prior to boot. The post-OTAFU equivalents are hosted on the Penn server.

End to end OTAFU Demo: https://www.youtube.com/watch?v=R9AgcVQNr38