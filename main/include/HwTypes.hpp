#pragma once

#include <string.h>

constexpr const char* HW_VER_0_0_1 = "0.0.1";
constexpr const char* HW_VER_0_0_2 = "0.0.2";

enum HwVer_e {
    HW_VER_0_0_1_E,
    HW_VER_UNKNOWN_E
};

struct HwVer_t {
    const char * name;
    HwVer_e hw_ver;
};

const HwVer_t ver[] = {
    { HW_VER_0_0_1, HW_VER_0_0_1_E },
    { nullptr,      HW_VER_UNKNOWN_E }   // table terminator
};
