#ifndef MAIN_H_
#define MAIN_H_

#include <opencv/cv.h>
#include <opencv/highgui.h>

#include "util.h"

void morphOps(cv::Mat &thresh);
void drawObject(int x, int y, cv::Mat &frame);
bool trackFilteredObject(int &x, int &y, cv::Mat threshold, cv::Mat &cameraFeed);

int user_input(ImageStruct *image_struct);

#endif