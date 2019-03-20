#pragma once
#include<string>

#define getHeadPointer(address, type, field) \
    ((type *)((char *)(address)-(unsigned long)(&((type *)0)->field)))


using int64 = long long;

bool isNumber(std::string);

int64 getCurrentTime();