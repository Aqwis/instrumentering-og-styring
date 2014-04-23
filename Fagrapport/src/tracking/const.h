#ifndef CONST_H_
#define CONST_H_

#include <string>

#include <opencv/cv.h>
#include <opencv/highgui.h>

const int PI = 3.14159265;

// Window names
const std::string windowOriginal = "Original Image";
const std::string windowLAB = "LAB Image";
const std::string windowThreshold = "Threshold Image";
const std::string windowMorph = "After Morphological Operations";
const std::string trackbarWindowName = "Trackbars";

// Dimensions
const int FRAME_WIDTH = 640;
const int FRAME_HEIGHT = 480;
const int MAX_NUM_OBJECTS = 50;
const int MIN_OBJECT_AREA = 20*20;
const int MAX_OBJECT_AREA = FRAME_HEIGHT*FRAME_WIDTH/1.5;

#endif