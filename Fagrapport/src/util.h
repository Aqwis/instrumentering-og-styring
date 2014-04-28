#ifndef _UTIL_H
#define _UTIL_H

#include <cstdlib>
#include <string>
#include <iostream>
#include <cmath>
#include <cstdio>

#include "com/com.h"

double copysign(double x, double y);
void sendSerialString(Serial *SIO, std::string message);

#endif
