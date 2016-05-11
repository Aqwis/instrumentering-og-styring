#include "main.h"
#include "util.h"
#ifdef _WIN32
#include "com/com.h"
#endif

using namespace cv;

// Thread handling
std::atomic<bool> input_thread_done(false);

// Serial I/O
/* Unfortunately, the current implementation of
   the serial communications only work in Windows,
   but it should be trivial to make it work
   under other operating systems as well.*/
#ifdef _WIN32
Serial *SIO;
#endif

// Features on/off
bool follow_object = false;

// Dimensions
const int FRAME_WIDTH = 640;
const int FRAME_HEIGHT = 480;
const int MAX_NUM_OBJECTS = 50;
const int MIN_OBJECT_AREA = 20*20;
const int MAX_OBJECT_AREA = FRAME_HEIGHT*FRAME_WIDTH/1.5;

// initial min and max LAB filter values. these will be changed using trackbars.
int L_MIN = 0;
int L_MAX = 256;
int A_MIN = 0;
int A_MAX = 256;
int B_MIN = 0;
int B_MAX = 256;

// Window names
const string windowOriginal = "Original Image";
const string windowLAB = "LAB Image";
const string windowThreshold = "Threshold Image";
const string windowMorph = "After Morphological Operations";
const string trackbarWindowName = "Trackbars";
const string trainingImage;

// Video data
Mat cameraFeed; // Matrix for the camera feed images

// Current object location
int x_location = 0;
int y_location = 0;

int angle_a = 90; // plane axis 1 (motor 1)
int angle_b = 15; // rotation in the plane
int angle_c = 10; // plane axis 2 (motor 2)

void on_trackbar( int, void* ) {
    // This function gets called whenever a
    // trackbar position is changed
}

void createTrackbars() {
    /* Creates the trackbar window and the trackbars
       for each component of the LAB color space. */
    
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

    /* Use some of the openCV drawing functions to draw large crosshairs
       on the tracked image! */

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

bool trackFilteredObject(int &x, int &y, Mat threshold, Mat &cameraFeed) {
    /* This function implements the algorithm that finds an object to track
       in an image taken from the camera feed. */
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
    /* Set trackbars to track the color in the
       center of the image. */
    const int l_margin = 50;
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

void onClick(int event, int x, int y, int flags, void *LAB_image_v) {
    /* If the user clicks on a pixel in the main window, the
       color of that pixel should be tracked. */

    if (event == EVENT_LBUTTONDOWN) {
        std::cout << "clicked!" << std::endl;
        const int l_margin = 50;
        const int a_margin = 10;
        const int b_margin = 10;

        Mat *LAB_image = (Mat*) LAB_image_v;

        unsigned char *triple = (unsigned char*) LAB_image->row(y).col(x).data;
        int l = triple[0];
        int a = triple[1];
        int b = triple[2];

        setTrackbarPos( "L_MIN", trackbarWindowName, l - l_margin );
        setTrackbarPos( "L_MAX", trackbarWindowName, l + l_margin );

        setTrackbarPos( "A_MIN", trackbarWindowName, a - a_margin );
        setTrackbarPos( "A_MAX", trackbarWindowName, a + a_margin );

        setTrackbarPos( "B_MIN", trackbarWindowName, b - b_margin );
        setTrackbarPos( "B_MAX", trackbarWindowName, b + b_margin );
    }
}

void draw_center_crosshair(Mat *image) {
    /* Draw crosshair in the center of the image. */
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
    /* Print the distance of the detected object
       from the center in degrees. */
    DegreeStruct *degr = degrees_from_center(x_location, y_location);
    std::cout << "Horizontal: " << degr->horizontal << std::endl;
    std::cout << "Vertical: " << degr->vertical << std::endl;
    delete degr;
}

void center_camera_simple() {
    /* Simple algorithm to move the camera towards the detected
       object. */

    DegreeStruct *degr = degrees_from_center(x_location, y_location);

    /* The number of degrees we move the camera towards the
       detected object depends on how far away it is, but we
       never move it the full distance at once, for increased
       stability. */
    if (std::abs(degr->horizontal) > 6) {
        degr->horizontal = copysign(2, degr->horizontal);
    } else if (std::abs(degr->horizontal) > 3) {
        degr->horizontal = copysign(1, degr->horizontal);
    } else if (std::abs(degr->horizontal) > 0) {
        degr->horizontal = 0;
    }
    if (std::abs(degr->vertical) > 6) {
        degr->vertical = copysign(2, degr->vertical);
    } else if (std::abs(degr->vertical) > 3) {
        degr->vertical = copysign(1, degr->vertical);
    } else if (std::abs(degr->vertical) > 0) {
        degr->vertical = 0;
    }

    /* If the object is outside the bounds of the servo
       motors, we do not move the camera. */
    if (std::abs(angle_a + degr->horizontal) > 175 || std::abs(angle_a + degr->horizontal) < 15) {
        return;
    } else if (std::abs(angle_c + degr->vertical) > 140 || std::abs(angle_c + degr->vertical) < 10) {
        return;
    }

    angle_a = angle_a + degr->horizontal;
    angle_c = angle_c + degr->vertical;

    sendSerialString(SIO, std::to_string(angle_a) + "a " + std::to_string(angle_c) + "c");
}

int user_input(ImageStruct *image_struct) {
    /* Create a simple command line to do
       simple tasks such as sending messages
       to the Arduino or tracking the color
       in the center of the image. */

	const std::string HELP_TEXT = "Commands:\n* help\n* center\n* exit\n* serial\n* distance\n* follow\n";

    std::string input = "";
    while (true) {
        /* Main loop for command line */
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
            /* Print help message. */
            std::cout << HELP_TEXT;
        } else if (command == "center") {
            /* Track color in center of image. */
            use_center_color(*(image_struct->LAB));
        } else if (command == "serial") {
            /* Send command to Arduino. */
			if (argument != "") {
	            sendSerialString(SIO, argument);
			} else {
				std::cout << "No argument provided!\n";
			}
        } else if (command == "distance") {
            /* Calculate distance of detected object
               from the center, in degrees. */
            print_object_degrees_from_center();
        } else if (command == "follow") {
            /* Should the servo motors track the
               detected object? */
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

    // Set up Serial I/O
    #ifdef _WIN32
    SIO = new Serial("\\\\.\\COM13");
    if (!SIO->IsConnected()) {
        std::cout << "Could not connect to COM device!" << std::endl;
    } else {
        sendSerialString(SIO, std::to_string(angle_a) + "a");
        sendSerialString(SIO, std::to_string(angle_b) + "b");
        sendSerialString(SIO, std::to_string(angle_c) + "c");
    }
    #endif
    
    // Some controls for functions in the program
    bool trackObjects = true;
    bool useMorphOps = true;

    Mat LAB; // Matrix for LAB image
    //Mat lab; // LAB color image
    Mat threshold; // Matrix for threshold threshold image
    bool objectFound = false; // Did we find a matching object?

    createTrackbars(); // Create the sliders for LAB filtering
    VideoCapture capture; // Video capture object for camera feed

    try 
    {
        capture.open(1); // Open capture object at location 1 (i.e. the second camera)
        cvtColor(cameraFeed,LAB,COLOR_BGR2Lab);
    }
    catch (cv::Exception e)  
    {
        capture.open(0); // If that doesn't work, use location 0 (often an integrated, immovable camera)
        cvtColor(cameraFeed,LAB,COLOR_BGR2Lab);
    }

    // For some reason, with some cameras, these settings
    // have no effect on the actual image size
    capture.set(CV_CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT);
    capture.set(CV_CAP_PROP_FRAME_WIDTH, FRAME_WIDTH);

    // Thread for console commands
    ImageStruct image_struct = {&cameraFeed, &LAB, &threshold};
    std::thread user_input_thread(user_input, &image_struct);

    // Mouse clicks in window should detect color
    namedWindow(windowThreshold, 1);
    namedWindow(windowOriginal, 2);
    namedWindow(windowLAB, 3);
    setMouseCallback(windowOriginal, onClick, &LAB);

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
