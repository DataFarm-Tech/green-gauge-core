#pragma once

#include <NTPClient.h>

extern NTPClient time_client;  // Declare the NTPClient object

bool start_sys_time();
bool get_sys_time(uint32_t * currentTime);