#pragma once

#include <Arduino.h>

void cmd_help();
void cmd_exit();
void cmd_reboot();
void cmd_queue();
void cmd_ping(const char* host);
void cmd_clear();
void cmd_threads();
