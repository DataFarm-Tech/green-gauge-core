#pragma once

void read_serial_cli(void* param);
void print_motd();
void print_help();
void handle_cmd(const char* cmd);
void trim_newline(char* str);