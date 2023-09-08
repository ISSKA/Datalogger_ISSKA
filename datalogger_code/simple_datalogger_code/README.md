# ISSKA Datalogger base code without wireless transmission
Base code for datalogger without wireless transmission. This code was rewritten in august 2023.

The conf.txt file on the SD card must have the following structure:
```
  300; //time step in seconds (must be either a multiple of 60, or it must be a divider of 60)
  1; //boolean value to choose if you want to set the RTC clock with time from SD card
  2023/03/31 16:05:00; //time to set the RTC if setRTC = 1
```

If you have any questions contact me per email at nicolas.schmid.6035@gmail.com
