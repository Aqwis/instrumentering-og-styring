
#include <sstream>
#include <string>
#include <iostream>
#include <opencv/cv.h>
#include <opencv/highgui.h>

using namespace cv;

// Global settings for whether the object is found
bool OBJECT_FOUND = false;
// Dimensions
const int FRAME_WIDTH = 640;
const int FRAME_HEIGHT = 480;
const int MAX_NUM_OBJECTS = 50;
const int MIN_OBJECT_AREA = 20*20;
const int MAX_OBJECT_AREA = FRAME_HEIGHT*FRAME_WIDTH/1.5;
// Counter for how many ms has passed
int MS_PASSED = 0;
// Tolerance limits
const int RIG_MOVE = 100; // pixels from the screen edge 
bool OBJECT_IN_BOX = false; 
// Servo angles
int SERVO_A = 90;
int SERVO_B = 90;
// initial min and max HSV filter values. these will be changed using trackbars.
// Decent values for tracking a red object;
// H: 0/249 S: 99/200 V: 62/122
int H_MIN = 0;
int H_MAX = 249;
int S_MIN = 99;
int S_MAX = 200;
int V_MIN = 62;
int V_MAX = 122;
// Window names
const string windowOriginal = "Original Image";
const string windowHSV = "HSV Image";
const string windowThreshold = "Threshold Image";
const string windowMorph = "After Morphological Operations";
const string trackbarWindowName = "Trackbars";

void on_trackbar( int sliderNewValue, void* object ) {
}

void createTrackbars() {
    
    namedWindow(trackbarWindowName, 0);
        
    //create memory to store trackbar name on window
    char TrackbarName[50];
    sprintf( TrackbarName, "H_MIN");
    sprintf( TrackbarName, "H_MAX");
    sprintf( TrackbarName, "S_MIN");
    sprintf( TrackbarName, "S_MAX");
    sprintf( TrackbarName, "V_MIN");
    sprintf( TrackbarName, "V_MAX");

    createTrackbar( "H_MIN", trackbarWindowName, &H_MIN, H_MAX, on_trackbar );
    createTrackbar( "H_MAX", trackbarWindowName, &H_MAX, H_MAX, on_trackbar );
    createTrackbar( "S_MIN", trackbarWindowName, &S_MIN, S_MAX, on_trackbar );
    createTrackbar( "S_MAX", trackbarWindowName, &S_MAX, S_MAX, on_trackbar );
    createTrackbar( "V_MIN", trackbarWindowName, &V_MIN, V_MAX, on_trackbar );
    createTrackbar( "V_MAX", trackbarWindowName, &V_MAX, V_MAX, on_trackbar );

}

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

string intToString(int number){
    std::stringstream ss;
    ss << number;
    return ss.str();
}

void drawObject(int x, int y, Mat &frame){

    //use some of the openCV drawing functions to draw crosshairs
    //on your tracked image!

    //UPDATE:JUNE 18TH, 2013
    //added 'if' and 'else' statements to prevent
    //memory errors from writing off the screen (ie. (-25,-25) is not within the window!)

    circle(frame,Point(x,y),20,Scalar(0,255,0),2);
    if(y-25>0)
    line(frame,Point(x,y),Point(x,y-25),Scalar(0,255,0),2);
    else line(frame,Point(x,y),Point(x,0),Scalar(0,255,0),2);
    if(y+25<FRAME_HEIGHT)
    line(frame,Point(x,y),Point(x,y+25),Scalar(0,255,0),2);
    else line(frame,Point(x,y),Point(x,FRAME_HEIGHT),Scalar(0,255,0),2);
    if(x-25>0)
    line(frame,Point(x,y),Point(x-25,y),Scalar(0,255,0),2);
    else line(frame,Point(x,y),Point(0,y),Scalar(0,255,0),2);
    if(x+25<FRAME_WIDTH)
    line(frame,Point(x,y),Point(x+25,y),Scalar(0,255,0),2);
    else line(frame,Point(x,y),Point(FRAME_WIDTH,y),Scalar(0,255,0),2);

    putText(frame,intToString(x)+","+intToString(y),Point(x,y+30),1,1,Scalar(0,255,0),2);

}

void trackFilteredObject(int &x, int &y, Mat threshold, Mat &cameraFeed) {
    
    Mat temp;
    threshold.copyTo(temp);
    // Vectors are needed for output of findContours
    vector< vector<Point> > contours;
    vector<Vec4i> hierarchy;
    // Find contours of filtered image using OpenCV findContours function
    findContours(temp, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE);
    // Use moments method to find our filtered object
    double refArea = 0;
    bool objectFound = false;
    if (hierarchy.size() > 0) { 
        int numObjects = hierarchy.size();
        // If number of objects greater than MAX_NUM_OBJECTS we have a noisy filter
        if (numObjects < MAX_NUM_OBJECTS) {
            for (int index=0; index >= 0; index = hierarchy[index][0]) {
                Moments moment = moments((cv::Mat)contours[index]);
                double area = moment.m00;

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
                else {
                    objectFound = false;
                }

            }

            OBJECT_FOUND = objectFound;

            // Let the user know an object has been found
            if (objectFound == true) {
                putText(cameraFeed, "Tracking Object", Point(0,50), 2, 1, Scalar(0, 255, 0), 2);
                drawObject(x, y, cameraFeed);
            }
            else {
                putText(cameraFeed, "TOO MUCH NOISE! ADJUST FILTER", Point(0,50), 1, 2, Scalar(0,0,255), 2);
            }

        }

    }


}

string tolerance(int x, int y, Mat &cameraFeed) {
    const int RIG_X_MIN = RIG_MOVE;
    const int RIG_X_MAX = FRAME_WIDTH - RIG_MOVE;
    const int RIG_Y_MIN = RIG_MOVE;
    const int RIG_Y_MAX = FRAME_HEIGHT - RIG_MOVE;

    bool inside_box = true;
    string output = "";

    // The X axis is governed by Servo A
    // 10 = far left, 180 = far right

    // if x is less than the minimum value, the rig needs to pan left
    if (x < RIG_X_MIN) {
        if (SERVO_A == 10) {
            putText(cameraFeed, "Servo A is at max pan left", Point(0,100), 1, 2, Scalar(0,0,255), 2);
        }
        else {
            SERVO_A = SERVO_A - 10;
            output += intToString(SERVO_A) + "a";
        }
        inside_box = false;
    }
    // if x is greater than the maximum value, the rig needs to pan right
    if (x > RIG_X_MAX) {
        if (SERVO_A == 180) {
            putText(cameraFeed, "Servo A is at max pan right", Point(0,100), 1, 2, Scalar(0,0,255), 2);
        }
        else {
            SERVO_A = SERVO_A + 10;
            output += intToString(SERVO_A) + "a";
        }
        inside_box = false;
    }

    // The Y axis is governed by Servo B
    // 10/180 = horizontal, 90 = vertical

    if (y < RIG_Y_MIN) {
        
    }

    // Set the global parameter for drawing the squares
    OBJECT_IN_BOX = inside_box;

    return output;
}

int main(int argc, char* argv[]) {
    
    // Some controls for functions in the program
    bool trackObjects = true;
    bool useMorphOps = true;
    // Matrix for the camera feed images
    Mat cameraFeed;
    // Matrix for HSV image
    Mat hsv;
    // Matrix for threshold threshold image
    Mat threshold;
    // x and y coordinates for the object
    int x=0, y=0;
    // Create the sliders for HSV filtering
    createTrackbars();
    // Video capture object for camera feed
    VideoCapture capture;
    // Open capture object at location zero (default for webcam)
    capture.open(0);

    capture.set(CV_CAP_PROP_FRAME_WIDTH,FRAME_WIDTH);
    capture.set(CV_CAP_PROP_FRAME_HEIGHT,FRAME_HEIGHT);

    while (1) {
        // Fetch frame from camera
        capture.read(cameraFeed);
        // Convert from BGR to HSV colorspace
        cvtColor(cameraFeed,hsv,COLOR_BGR2HSV);
        // Filter HSV image between values and store filtered images to threshold materix
        inRange(hsv, Scalar(H_MIN, S_MIN, V_MIN), Scalar(H_MAX, S_MAX, V_MAX), threshold);
        
        // Morph to smooth image
        if (useMorphOps) {
            morphOps(threshold);
        }

        // Track objects
        if (trackObjects) {
            trackFilteredObject(x, y, threshold, cameraFeed);
        }

        MS_PASSED+=10;        
        // Servo adjustments
        if (MS_PASSED == 100) {
            if (OBJECT_FOUND) {
                string output = tolerance(x, y, cameraFeed);
                if (!output.empty()) {
                    std::cout << "Servo output: " << tolerance(x, y, cameraFeed) << std::endl; 
                }
            }
            MS_PASSED = 0;
        }


        // Set the appropriate color on the square on the screen indicating object tracking tolerance
        if (OBJECT_IN_BOX) {
            rectangle(cameraFeed, Point(RIG_MOVE ,RIG_MOVE), Point(FRAME_WIDTH-RIG_MOVE, FRAME_HEIGHT-RIG_MOVE), Scalar(0, 255, 0), 1);
        }
        else {
            rectangle(cameraFeed, Point(RIG_MOVE ,RIG_MOVE), Point(FRAME_WIDTH-RIG_MOVE, FRAME_HEIGHT-RIG_MOVE), Scalar(0, 0, 255), 1);
        }

        // Show frames
        imshow(windowThreshold, threshold);
        imshow(windowOriginal, cameraFeed);
        imshow(windowHSV, hsv);
        
        /*
        std::vector<int> past {-1, -1, -1, -1, -1, -1};
        std::vector<int> current { H_MIN, H_MAX, S_MIN, S_MAX, V_MIN, V_MAX };

        if (current != past) {
            std::vector<int> past { H_MIN, H_MAX, S_MIN, S_MAX, V_MIN, V_MAX };
            //std::cout << "min/max - H: " << H_MIN << "/" << H_MAX << " S: " << S_MIN << "/" << S_MAX << " V: " << V_MIN << "/" << V_MAX << '\n';
            std::cout << "SERVO_A: " << SERVO_A << " - SERVO_B: " << SERVO_B << '\n';
        }
        */

        waitKey(10);

    }



}
