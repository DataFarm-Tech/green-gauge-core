#pragma once

#include <cstdint>

constexpr char NODE_ID[] = "nod123";
constexpr char BATT_URI[] = "coap://45.79.239.100/battery";
constexpr char DATA_URI[] = "coap://45.79.239.100/reading";
constexpr char ACT_URI[] = "coap://45.79.239.100/activate";

constexpr char BATT_TAG[] = "BatteryPacket";
constexpr char DATA_TAG[] = "DataPacket";
constexpr char ACT_TAG[] = "ActivatePacket";

constexpr int sleep_time_sec = 60 * 60 * 1;
