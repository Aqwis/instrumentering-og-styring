#ifndef _MAIN_H
#define _MAIN_H

#include <cstdlib>
#include <atomic>
#include <string>
#include <sstream>
#include <iostream>
#include <thread>
#include <cmath>
#include <cstdio>

#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/nonfree/features2d.hpp>

#include "util.h"

const int PI = 3.14159265;
extern std::atomic<bool> input_thread_done;

struct ImageStruct {
	cv::Mat *cameraFeed;
	cv::Mat *LAB;
	cv::Mat *threshold;
};

struct DegreeStruct {
    int horizontal;
    int vertical;
};

//// FUNCTION DECLARATIONS ////

void on_trackbar( int, void* );
void createTrackbars();

void morphOps(cv::Mat &thresh);
void drawObject(int x, int y, cv::Mat &frame);
bool trackFilteredObject(int &x, int &y, cv::Mat threshold, cv::Mat &cameraFeed);

int user_input(ImageStruct *image_struct);

DegreeStruct *degrees_from_center(int x, int y);
void print_object_degrees_from_center();

//// DEFINED IN main.cpp ////

// Dimensions
extern const int FRAME_WIDTH;
extern const int FRAME_HEIGHT;
extern const int MAX_NUM_OBJECTS;
extern const int MIN_OBJECT_AREA;
extern const int MAX_OBJECT_AREA;

// initial min and max LAB filter values. these will be changed using trackbars.
extern int H_MIN;
extern int H_MAX;
extern int S_MIN;
extern int S_MAX;
extern int V_MIN;
extern int V_MAX;

// Window names
extern const std::string windowOriginal;
extern const std::string windowLAB;
extern const std::string windowThreshold;
extern const std::string windowMorph;
extern const std::string trackbarWindowName;

#endif
