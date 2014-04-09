#include "main.h"
#include "com.h"

using namespace cv;

// Serial I/O
Serial *SIO;

// Thread handling
std::atomic<bool> input_thread_done(false);

// Features on/off
bool follow_object = false;

// Dimensions
const int FRAME_WIDTH = 640;
const int FRAME_HEIGHT = 480;
const int MAX_NUM_OBJECTS = 50;
const int MIN_OBJECT_AREA = 20*20;
const int MAX_OBJECT_AREA = FRAME_HEIGHT*FRAME_WIDTH/1.5;

// initial min and max HSV filter values. these will be changed using trackbars.
int H_MIN = 0;
int H_MAX = 256;
int S_MIN = 0;
int S_MAX = 256;
int V_MIN = 0;
int V_MAX = 256;

// Window names
const string windowOriginal = "Original Image";
const string windowHSV = "HSV Image";
const string windowThreshold = "Threshold Image";
const string windowMorph = "After Morphological Operations";
const string trackbarWindowName = "Trackbars";

// Video data
Mat cameraFeed; // Matrix for the camera feed images

bool objectFound = false; // Has an object been detected?

// Current object location
int x_location = 0;
int y_location = 0;

int angle_a = 90; // plane axis 1 (motor 1)
int angle_b = 15; // rotation in the plane
int angle_c = 10; // plane axis 2 (motor 2)

struct ImageStruct {
    Mat *cameraFeed;
    Mat *hsv;
    Mat *threshold;
};

struct DegreeStruct {
    int horizontal;
    int vertical;
};

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
    sprintf( TrackbarName, "H_MIN", H_MIN);
    sprintf( TrackbarName, "H_MAX", H_MAX);
    sprintf( TrackbarName, "S_MIN", S_MIN);
    sprintf( TrackbarName, "S_MAX", S_MAX);
    sprintf( TrackbarName, "V_MIN", V_MIN);
    sprintf( TrackbarName, "V_MAX", V_MAX);

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

void drawObject(int x, int y, Mat &frame) {

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

    putText(frame,std::to_string(x)+","+std::to_string(y),Point(x,y+30),1,1,Scalar(0,255,0),2);

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

            // Let the user know an object has been found
            if (objectFound == true) {
                putText(cameraFeed, "Tracking Object", Point(0,50), 2, 1, Scalar(0, 255, 0), 2);
                drawObject(x, y, cameraFeed);
                return;
            }
            else {
                putText(cameraFeed, "TOO MUCH NOISE! ADJUST FILTER", Point(0,50), 1, 2, Scalar(0,0,255), 2);
            }
        }
    }
    objectFound = false;
}

double use_center_color(Mat hsv_image) {
    const int h_margin = 10;
    const int s_margin = 10;
    const int v_margin = 35;

    Size image_size = hsv_image.size();

    int vertical_center = image_size.height/2;
    int horizontal_center = image_size.width/2;
    unsigned char *center_triple = (unsigned char*) hsv_image.row(vertical_center).col(horizontal_center).data;

    int center_h = center_triple[0];
    int center_s = center_triple[1];
    int center_v = center_triple[2];

    //std::cout << "HSV: " << center_h << "," << center_s << "," << center_v << std::endl;

    setTrackbarPos( "H_MIN", trackbarWindowName, center_h - h_margin );
    setTrackbarPos( "H_MAX", trackbarWindowName, center_h + h_margin );

    setTrackbarPos( "S_MIN", trackbarWindowName, center_s - s_margin );
    setTrackbarPos( "S_MAX", trackbarWindowName, center_s + s_margin );

    setTrackbarPos( "V_MIN", trackbarWindowName, center_v - v_margin );
    setTrackbarPos( "V_MAX", trackbarWindowName, center_v + v_margin );

    return 0;
}

void sendSerialString(std::string message) {
    bool success = SIO->WriteData((char *) message.c_str(), message.length());
    std::cout << "Wrote " << message << std::endl;
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
            use_center_color(*(image_struct->hsv));
        } else if (command == "serial") {
            sendSerialString(argument);
        } else if (command == "distance") {
            print_object_degrees_from_center();
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

void draw_center_crosshair(Mat *image) {
    int screenHeight;
    int screenWidth;

    Size image_size = image->size();
    screenHeight = image_size.height;
    screenWidth = image_size.width;
    line(*image, Point((screenWidth/2)-20, screenHeight/2), Point((screenWidth/2)+20, screenHeight/2), Scalar(0,0,0), 2, 8);
    line(*image, Point(screenWidth/2, (screenHeight/2)-20), Point(screenWidth/2, (screenHeight/2)+20), Scalar(0,0,0), 2, 8);
}

DegreeStruct *degrees_from_center(int x, int y) {
    // Compute the number of degrees away from the center
    // the given pixel location is

    const int H_FOV = 60; // horizontal field of view of camera
    const int V_FOV = 45; // vertical field of view of camera
    DegreeStruct *degrees = new DegreeStruct();

    Size image_size = cameraFeed.size();
    int screenHeight = image_size.height;
    int screenWidth = image_size.width;

    float horizontal_degrees_per_pixel = (float) H_FOV/screenWidth;
    float vertical_degrees_per_pixel = (float) V_FOV/screenHeight;

    int horizontal_center = screenWidth/2;
    int vertical_center = screenHeight/2;

    degrees->horizontal = horizontal_degrees_per_pixel*(x-horizontal_center);
    degrees->vertical = vertical_degrees_per_pixel*(vertical_center-y);

    return degrees;
}

void print_object_degrees_from_center() {
    DegreeStruct *degr = degrees_from_center(x_location, y_location);
    std::cout << "Horizontal: " << degr->horizontal << std::endl;
    std::cout << "Vertical: " << degr->vertical << std::endl;
    delete degr;
}

void center_camera() {
    DegreeStruct *degr = degrees_from_center(x_location, y_location);

    // Distance from center
    Size image_size = cameraFeed.size();
    int distance_y = (y_location - image_size.height/2);
    int distance_x = (x_location - image_size.width/2);

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

int main(int argc, char* argv[]) {

    // Set up Serial I/O
    SIO = new Serial("\\\\.\\COM13");
    if (!SIO->IsConnected()) {
        std::cout << "Could not connect to COM device!" << std::endl;
    } else {
        sendSerialString(std::to_string(angle_a) + "a");
        sendSerialString(std::to_string(angle_b) + "b");
        sendSerialString(std::to_string(angle_c) + "c");
    }
    
    // Some controls for functions in the program
    bool trackObjects = true;
    bool useMorphOps = true;

    Mat hsv; // Matrix for HSV image
    Mat threshold; // Matrix for threshold threshold image

    createTrackbars(); // Create the sliders for HSV filtering
    VideoCapture capture; // Video capture object for camera feed
    capture.open(1); // Open capture object at location 0 (i.e. the first camera)

    // For some reason, with some cameras, these settings
    // have no effect on the actual image size
    capture.set(CV_CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT);
    capture.set(CV_CAP_PROP_FRAME_WIDTH, FRAME_WIDTH);

    // Thread for console commands
    ImageStruct image_struct = {&cameraFeed, &hsv, &threshold};
    std::thread user_input_thread(user_input, &image_struct);

    while (!input_thread_done) {
        capture.read(cameraFeed); // Fetch frame from camera
        cvtColor(cameraFeed,hsv,COLOR_BGR2HSV); // Convert from BGR to HSV colorspace
        // Filter HSV image between values and store filtered images to threshold materix
        inRange(hsv, Scalar(H_MIN, S_MIN, V_MIN), Scalar(H_MAX, S_MAX, V_MAX), threshold);

        // Draw crosshair in the middle of the image
        draw_center_crosshair(&cameraFeed);
        
        // Morph to smooth image
        if (useMorphOps) {
            morphOps(threshold);
        }

        // Track objects
        if (trackObjects) {
            trackFilteredObject(x_location, y_location, threshold, cameraFeed);
        }

        // Center camera on tracked object
        if (follow_object && objectFound) {
            center_camera_simple();
        }

        // Show frames
        imshow(windowThreshold, threshold);
        imshow(windowOriginal, cameraFeed);
        imshow(windowHSV, hsv);

        waitKey(100);

    }

    user_input_thread.detach();
    return 0;
}
