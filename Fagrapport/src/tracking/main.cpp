#include <cstdlib>
#include <string>
#include <sstream>
#include <iostream>
#include <thread>
#include <cmath>
#include <cstdio>

#include <opencv/cv.h>
#include <opencv/highgui.h>

#include "main.h"
#include "util.h"
#include "const.h"

using namespace cv;

// Features on/off
bool follow_object = false;

// Video data
Mat cameraFeed; // Matrix for the camera feed images

// Current object location
int x_location = 0;
int y_location = 0;

void morphOps(Mat &thresh) {
    //create structuring element that will be used to "dilate" and "erode" image.
    //the element chosen here is a 3px by 3px rectangle
    Mat erodeElement = getStructuringElement(MORPH_RECT, Size(3,3));
    Mat dilateElement = getStructuringElement(MORPH_RECT, Size(8,8));

    erode(thresh, thresh, erodeElement);
    erode(thresh, thresh, erodeElement);
    
    dilate(thresh, thresh, dilateElement);
    dilate(thresh, thresh, dilateElement);
}

bool trackFilteredObject(int &x, int &y, Mat threshold, Mat &cameraFeed) {
    Mat temp;
    Moments moment;
    int numObjects;
    double area;

    bool objectFound = false;
    double refArea = 0;

    // Vectors are needed for output of findContours
    vector<vector<Point>> contours;
    vector<Vec4i> hierarchy;

    // Find contours of filtered image using OpenCV findContours function
    threshold.copyTo(temp);
    findContours(temp, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE);

    // Use moments method to find our filtered object
    numObjects = hierarchy.size();

    if (numObjects > 0 && numObjects < MAX_NUM_OBJECTS) { 
        // If number of objects greater than MAX_NUM_OBJECTS we have a noisy filter
        for (int index=0; index >= 0; index = hierarchy[index][0]) {
            moment = moments((cv::Mat) contours[index]);
            area = moment.m00;

            // If the area is less than 20*20 pixels it's probably just noise.
            // If the area is the same as 2/3 of the images size, probably a bad filter
            // We want only the object with the largest area so we can save a reference area each
            // iteration and compare it to the area in the next iteration
            if (area > MIN_OBJECT_AREA && area < MAX_OBJECT_AREA && area > refArea) {
                x = moment.m10/area;
                y = moment.m01/area;
                objectFound = true;
                refArea = area;
            }
        }
    }

    // Let the user know an object has been found
    if (objectFound == true) {
        putText(cameraFeed, "Tracking Object", Point(0,50), 2, 1, Scalar(0, 255, 0), 2);
        drawObject(x, y, cameraFeed);
    } else {
        putText(cameraFeed, "TOO MUCH NOISE! ADJUST FILTER", Point(0,50), 1, 2, Scalar(0,0,255), 2);
    }

    return objectFound;
}

double use_center_color(Mat LAB_image) {
    const int l_margin = 10;
    const int a_margin = 10;
    const int b_margin = 10;

    Size image_size = LAB_image.size();

    int vertical_center = image_size.height/2;
    int horizontal_center = image_size.width/2;
    unsigned char *center_triple = (unsigned char*) LAB_image.row(vertical_center).col(horizontal_center).data;

    int center_l = center_triple[0];
    int center_a = center_triple[1];
    int center_b = center_triple[2];

    //std::cout << "LAB: " << center_h << "," << center_s << "," << center_v << std::endl;

    setTrackbarPos( "L_MIN", trackbarWindowName, center_l - l_margin );
    setTrackbarPos( "L_MAX", trackbarWindowName, center_l + l_margin );

    setTrackbarPos( "A_MIN", trackbarWindowName, center_a - a_margin );
    setTrackbarPos( "A_MAX", trackbarWindowName, center_a + a_margin );

    setTrackbarPos( "B_MIN", trackbarWindowName, center_b - b_margin );
    setTrackbarPos( "B_MAX", trackbarWindowName, center_b + b_margin );

    return 0;
}

void center_camera() {
    DegreeStruct *degr = degrees_from_center(x_location, y_location);

    // Distance from center
    int distance_y = (y_location - screenHeight/2);
    int distance_x = (x_location - screenWidth/2);

    double rotation_angle_abs = angle_b - atan2(distance_y, distance_x) * 180 / PI;
    double radius_abs = angle_a - sqrt(pow(degr->horizontal, 2) + pow(degr->vertical, 2));

    int rotation_angle = floor(copysign(rotation_angle_abs, distance_y));
    int radius = floor(copysign(radius_abs, distance_x));

    angle_b = rotation_angle;
    angle_a = radius_abs;

    // Adjust angles a and b
    sendSerialString(std::to_string(rotation_angle) + "b " + std::to_string(radius) + "a");
}

void center_camera_simple() {
    DegreeStruct *degr = degrees_from_center(x_location, y_location);

    angle_a = angle_a + degr->horizontal;
    angle_c = angle_c + degr->vertical;

    sendSerialString(std::to_string(angle_a) + "a " + std::to_string(angle_c) + "c");
}

int user_input(ImageStruct *image_struct) {
    const std::string HELP_TEXT = "Commands:\n* help\n* center\n* exit\n* serial\n* distance\n* follow\n";

    std::string input = "";
    while (true) {
        std::cout << ">";
        std::getline(std::cin, input);
        int first_space_index = input.find_first_of(" ");
        std::string command = input.substr(0, first_space_index);
        std::string argument = input.substr(first_space_index+1);
        // Interpret input
        if (command == "exit") {
            input_thread_done = true;
            return 0;
        } else if (command == "help" || command == "?") {
            std::cout << HELP_TEXT;
        } else if (command == "center") {
            use_center_color(*(image_struct->LAB));
        } else if (command == "serial") {
            sendSerialString(argument);
        } else if (command == "distance") {
            print_object_degrees_from_center(x_location, y_location);
        } else if (command == "follow") {
            follow_object = true;
        } else {
            std::cout << "Unrecognized input!\n";
        }
        input = "";
    }
    input_thread_done = true;
    return 1;
}

int main(int argc, char* argv[]) {

    bool sio_success = setup_serial_io();
    
    // Some controls for functions in the program
    bool trackObjects = true;
    bool useMorphOps = true;

    Mat LAB; // Matrix for LAB image
    Mat lab; // LAB color image
    Mat threshold; // Matrix for threshold threshold image
    bool objectFound = false; // Did we find a matching object?

    createTrackbars(); // Create the sliders for LAB filtering
    VideoCapture capture; // Video capture object for camera feed
    capture.open(1); // Open capture object at location 0 (i.e. the first camera)

    // Get actual image size
    Size image_size = cameraFeed.size();
    screenHeight = image_size.height;
    screenWidth = image_size.width;

    // For some reason, with some cameras, these settings
    // have no effect on the actual image size
    capture.set(CV_CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT);
    capture.set(CV_CAP_PROP_FRAME_WIDTH, FRAME_WIDTH);

    // Thread for console commands
    ImageStruct image_struct = {&cameraFeed, &LAB, &threshold};
    std::thread user_input_thread(user_input, &image_struct);

    while (!input_thread_done) {
        capture.read(cameraFeed); // Fetch frame from camera
        cvtColor(cameraFeed,LAB,COLOR_BGR2Lab); // Convert from BGR to LAB colorspace
        // Filter LAB image between values and store filtered images to threshold materix
        inRange(LAB, Scalar(L_MIN, A_MIN, B_MIN), Scalar(L_MAX, A_MAX, B_MAX), threshold);

        // Draw crosshair in the middle of the image
        draw_center_crosshair(&cameraFeed);
        
        // Morph to smooth image
        if (useMorphOps) {
            morphOps(threshold);
        }

        // Track objects
        if (trackObjects) {
            objectFound = trackFilteredObject(x_location, y_location, threshold, cameraFeed);
        }

        // Center camera on tracked object
        if (follow_object && objectFound) {
            center_camera_simple();
        }

        // Show frames
        imshow(windowThreshold, threshold);
        imshow(windowOriginal, cameraFeed);
        imshow(windowLAB, LAB);

        waitKey(20);

    }

    user_input_thread.detach();
    return 0;
}
