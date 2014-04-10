#include <cstdlib>
#include <string>
#include <sstream>
#include <iostream>
#include <thread>
#include <atomic>
#include <cmath>
#include <cstdio>

#include <opencv/cv.h>
#include <opencv/highgui.h>

struct ImageStruct;
struct DegreeStruct;

//// FUNCTION DECLARATIONS ////

const int PI = 3.14159265;

void on_trackbar( int, void* );
void createTrackbars();

void morphOps(cv::Mat &thresh);
void drawObject(int x, int y, cv::Mat &frame);
bool trackFilteredObject(int &x, int &y, cv::Mat threshold, cv::Mat &cameraFeed);

int user_input(ImageStruct *image_struct);

DegreeStruct *degrees_from_center(int x, int y);
void print_object_degrees_from_center();

//// DEFINED IN main.cpp ////

// Thread handling
extern std::atomic<bool> input_thread_done;

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
