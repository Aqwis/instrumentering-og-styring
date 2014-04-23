#include <string>
#include <sstream>
#include <iostream>
#include <cmath>
#include <atomic>
#include <cstdio>

#include <opencv/cv.h>
#include <opencv/highgui.h>

#include "util.h"
#include "const.h"
#include "com.h"

using namespace cv;

Serial *SIO; // Serial I/O

std::atomic<bool> input_thread_done(false); // Thread handling

int angle_a = 90; // Plane axis 1 (motor 1)
int angle_b = 15; // Rotation in the plane
int angle_c = 10; // Plane axis 2 (motor 2)

// initial min and max LAB filter values. these will be changed using trackbars.
int L_MIN = 0;
int L_MAX = 256;
int A_MIN = 0;
int A_MAX = 256;
int B_MIN = 0;
int B_MAX = 256;

int screenHeight = 1;
int screenWidth = 1;

double copysign(double x, double y) {
    if (y >= 0) {
        return abs(x);
    } else {
        return -abs(x);
    }
}

void on_trackbar( int, void* ) {
    // This function gets called whenever a
    // trackbar position is changed
}

void createTrackbars() {
    
    namedWindow(trackbarWindowName, 0);
        
    //create memory to store trackbar name on window
    char TrackbarName[50];
    sprintf( TrackbarName, "L_MIN", L_MIN);
    sprintf( TrackbarName, "L_MAX", L_MAX);
    sprintf( TrackbarName, "A_MIN", A_MIN);
    sprintf( TrackbarName, "A_MAX", A_MAX);
    sprintf( TrackbarName, "B_MIN", B_MIN);
    sprintf( TrackbarName, "B_MAX", B_MAX);

    createTrackbar( "L_MIN", trackbarWindowName, &L_MIN, L_MAX, on_trackbar );
    createTrackbar( "L_MAX", trackbarWindowName, &L_MAX, L_MAX, on_trackbar );
    createTrackbar( "A_MIN", trackbarWindowName, &A_MIN, A_MAX, on_trackbar );
    createTrackbar( "A_MAX", trackbarWindowName, &A_MAX, A_MAX, on_trackbar );
    createTrackbar( "B_MIN", trackbarWindowName, &B_MIN, B_MAX, on_trackbar );
    createTrackbar( "B_MAX", trackbarWindowName, &B_MAX, B_MAX, on_trackbar );

}

void draw_center_crosshair(Mat *image) {
    int _screenHeight;
    int _screenWidth;

    Size image_size = image->size();
    _screenHeight = image_size.height;
    _screenWidth = image_size.width;
    line(*image, Point((_screenWidth/2)-20, _screenHeight/2), Point((_screenWidth/2)+20, _screenHeight/2), Scalar(0,0,0), 2, 8);
    line(*image, Point(_screenWidth/2, (_screenHeight/2)-20), Point(_screenWidth/2, (_screenHeight/2)+20), Scalar(0,0,0), 2, 8);
}

void drawObject(int x, int y, Mat &frame) {

    //use some of the openCV drawing functions to draw crosshairs
    //on your tracked image!

    //UPDATE:JUNE 18TH, 2013
    //added 'if' and 'else' statements to prevent
    //memory errors from writing off the screen (ie. (-25,-25) is not within the window!)

    circle(frame, Point(x,y), 20, Scalar(0,255,0), 2);

    if (y-25 > 0) {
        line(frame, Point(x,y), Point(x, y-25), Scalar(0,255,0), 2);
    } else {
        line(frame, Point(x,y), Point(x,0), Scalar(0,255,0), 2);
    }

    if (y+25 < FRAME_HEIGHT) {
        line(frame, Point(x,y), Point(x, y+25), Scalar(0,255,0), 2);
    } else {
        line(frame, Point(x,y), Point(x, FRAME_HEIGHT), Scalar(0,255,0), 2);
    }

    if (x-25 > 0) {
        line(frame, Point(x,y), Point(x-25, y), Scalar(0,255,0), 2);
    } else {
        line(frame, Point(x,y), Point(0, y), Scalar(0,255,0), 2);
    }

    if (x+25<FRAME_WIDTH) {
        line(frame, Point(x,y), Point(x+25, y), Scalar(0,255,0), 2);
    } else {
        line(frame, Point(x,y), Point(FRAME_WIDTH, y), Scalar(0,255,0), 2);
    }

    putText(frame, std::to_string(x) + "," + std::to_string(y), Point(x,y + 30), 1, 1, Scalar(0,255,0), 2);

}

void sendSerialString(std::string message) {
    bool success = SIO->WriteData((char *) message.c_str(), message.length());
    std::cout << "Wrote " << message << std::endl;
}

bool setup_serial_io() {
    SIO = new Serial("\\\\.\\COM13");

    if (!SIO->IsConnected()) {
        std::cout << "Could not connect to COM device!" << std::endl;
        return true;
    } else {
        // Use default angles
        sendSerialString(std::to_string(angle_a) + "a");
        sendSerialString(std::to_string(angle_b) + "b");
        sendSerialString(std::to_string(angle_c) + "c");
        return false;
    }
}

DegreeStruct *degrees_from_center(int x, int y) {
    // Compute the number of degrees away from the center
    // the given pixel location is

    const int H_FOV = 60; // horizontal field of view of camera
    const int V_FOV = 45; // vertical field of view of camera
    DegreeStruct *degrees = new DegreeStruct();

    float horizontal_degrees_per_pixel = (float) H_FOV/screenWidth;
    float vertical_degrees_per_pixel = (float) V_FOV/screenHeight;

    int horizontal_center = screenWidth/2;
    int vertical_center = screenHeight/2;

    degrees->horizontal = horizontal_degrees_per_pixel*(x-horizontal_center);
    degrees->vertical = vertical_degrees_per_pixel*(vertical_center-y);

    return degrees;
}

void print_object_degrees_from_center(int x_location, int y_location) {
    DegreeStruct *degr = degrees_from_center(x_location, y_location);
    std::cout << "Horizontal: " << degr->horizontal << std::endl;
    std::cout << "Vertical: " << degr->vertical << std::endl;
    delete degr;
}
