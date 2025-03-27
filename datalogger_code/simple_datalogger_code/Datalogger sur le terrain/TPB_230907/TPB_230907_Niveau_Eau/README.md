# ISSKA Datalogger base code without wireless transmission
Base code for datalogger without wireless transmission. This code was rewritten in august 2023.

The conf.txt file on the SD card must have the following structure:
```
  300; //time step in seconds (must be either a multiple of 60, or it must be a divider of 60)
  1; //boolean value to choose if you want to set the RTC clock with time from SD card
  2023/03/31 16:05:00; //time to set the RTC if setRTC = 1
```

To set the time on a RTC, change the boolean value in the conf.txt file to 1, and write the time at which you will reboot the datalogger in the third row of the conf.txt file. Restart the datalogger at the exact time that you wrote in teh conf.txt file. After the first reboot, change the boolean value in the conf.txt file to 0 to avoid that the time is changed each time you reboot the datalogger.

If you have any questions contact me per email at nicolas.schmid.6035@gmail.com
