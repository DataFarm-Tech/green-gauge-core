#pragma once
#include <stdbool.h>

class OTAUpdater {
public:
    bool update(const char* url);
    static void printInfo();
};
