# NoteCard_LTE_DataLogger
Datalogger with a notecard that sends data through LTE
# Description
The notecard sends the data to the notehub platform each time the MAX_MEASUREMENTS number is reached. The data are then routed to another platform called "IoTPlotter". A Python script is available to download the data from this platform.
# Sending of the data
All the data are sent to the notehub through a JSON object. The JSON library used in the arduino notecard library is the "n_cjson" library (available here: https://github.com/blues/note-c/blob/master/n_cjson.c). More explanation of this library and some examples can be found following this link https://github.com/DaveGamble/cJSON.
note: In the second link, cJSON_ is used before all methods. In the first link, this therm is substituted by J.
