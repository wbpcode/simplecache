#include "cache-tool.h"
#include <sstream>
#include <string>
#include <chrono>

bool isNumber(std::string str) {
    std::stringstream sin(str);
    long long n;
    char p;
    if (!(sin >> n)) return false;
    if (sin >> p)    return false; 
    else             return true; 
}

int64 getCurrentTime() {
    return std::chrono::time_point_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now())
        .time_since_epoch()
        .count();
}