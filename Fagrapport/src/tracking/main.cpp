#include "main.h"

using namespace cv;

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

int user_input() {
	std::string COMMANDS_[] = {"pick"};
	const std::set<std::string> COMMANDS(COMMANDS_, COMMANDS_ + sizeof(COMMANDS_)/sizeof(COMMANDS_[0]));
	const std::string HELP_TEXT = "Commands:\n* exit\n*pick\n";

    std::string input = "";
    while (true) {
        std::cout << ">";
        std::cin >> input;
        if (input == "exit") {
            clean_up_and_exit(0);
		} else if (COMMANDS.find(input) != COMMANDS.end()) {
            std::cout << "INPUT!";
        }
		std::cout << "\n";
        input = "";
    }
    return 1;
}

void clean_up_and_exit(int success_value) {
	std::exit(success_value);
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
    capture.open(1);

    capture.set(CV_CAP_PROP_FRAME_HEIGHT,FRAME_HEIGHT);
    capture.set(CV_CAP_PROP_FRAME_WIDTH,FRAME_WIDTH);

    // Thread for console commands
    std::thread user_input_thread(user_input);

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

        // Show frames
        imshow(windowThreshold, threshold);
        imshow(windowOriginal, cameraFeed);
        imshow(windowHSV, hsv);

        waitKey(33);

    }

}
