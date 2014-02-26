#include <iostream>
#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>

using namespace cv;
using namespace std;

int main(int argc, char** argv)
{
    Mat image;
    image = imread("cars1.jpg", CV_LOAD_IMAGE_COLOR);

    if (!image.data) {
        cout << "Could not open image." << endl;
    }

    return 0;
}
