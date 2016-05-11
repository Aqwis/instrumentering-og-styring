Readme
===========

This code implements the object detection and tracking
algorithms described in our Experts in Team report.

Unfortunately, while object detection should work under any
OS, due to time constraints and the operating systems
used by most group members, serial communication will
only work on Windows.

To compile:
```cd build/<os>
cmake ../..
make
make install```

OpenCV 2.4.8 or newer is required to compile.

The code used for communication on the Arduino is given in the
file ArduinoKode.cpp.
