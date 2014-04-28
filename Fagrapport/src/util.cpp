#include "util.h"
#include "com/com.h"

double copysign(double x, double y) {
    if (y >= 0) {
        return abs(x);
    } else {
        return -abs(x);
    }
}

void sendSerialString(Serial *SIO,std::string message) {
    #ifdef _WIN32
    bool success = SIO->WriteData((char *) message.c_str(), message.length());
    std::cout << "Wrote " << message << std::endl;
    #endif
}
