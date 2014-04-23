#ifndef UTIL_H_
#define UTIL_H_

#include <atomic>
#include <string>

#include <opencv/cv.h>
#include <opencv/highgui.h>

using namespace cv;

extern std::atomic<bool> input_thread_done;

extern int angle_a; // Plane axis 1 (motor 1)
extern int angle_b; // Rotation in the plane
extern int angle_c; // Plane axis 2 (motor 2)

// initial min and max LAB filter values. these will be changed using trackbars.
extern int L_MIN;
extern int L_MAX;
extern int A_MIN;
extern int A_MAX;
extern int B_MIN;
extern int B_MAX;

// Actual screen height/width -- don't trust the constants!
extern int screenHeight;
extern int screenWidth;

struct ImageStruct {
    Mat *cameraFeed;
    Mat *LAB;
    Mat *threshold;
};

struct DegreeStruct {
    int horizontal;
    int vertical;
};

double copysign(double x, double y);

void on_trackbar( int, void* );
void createTrackbars();

void draw_center_crosshair(Mat *image);
void drawObject(int x, int y, Mat &frame);

void sendSerialString(std::string message);
bool setup_serial_io();

DegreeStruct *degrees_from_center(int x, int y);
void print_object_degrees_from_center(int x_location, int y_location);

#endif
