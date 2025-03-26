# Description
Code for datalogger with MS8607 (PHT of air) and MS5803 (pressure and temperature of water) sensors.
The ground of the notecard is connected to the ground of the TinyPico by a transistor. The gate of this transistor is connected to pin 4.

# Config file 
The conf.txt file on the SD card is read to set the time step of the measurements. In this file we can also choose to set the RTC time with time received from the notecard thorugh a request (1; to set the time or 0; to let the current RTC time). This file needs to be written as follows:(example with measurement step of 1800 seconds and time request to set the RTC at the first run).<br/>
1800;<br/>
1;
