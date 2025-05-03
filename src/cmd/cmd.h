#pragma once

#include <Arduino.h>

typedef enum {
    CMD_HELP,
    CMD_EXIT,
    CMD_REBOOT,
    CMD_QUEUE,
    CMD_PING,
    CMD_CLEAR,
    CMD_THREADS,
    CMD_TIME,
    CMD_TEARDOWN,
    CMD_IPCONFIG,
    CMD_QADD,
    CMD_APPLY,
    CMD_KEY,
    CMD_CLEAR_CONFIG,
    CMD_LIST,
    CMD_STOP_THREAD,
    CMD_UNKNOWN
} cli_cmd;

void cmd_help();
void cmd_exit();
void cmd_reboot();
void cmd_queue();
void cmd_ping(const char* host);
void cmd_clear();
void cmd_threads();
void cmd_time();
void cmd_teardown();
void cmd_ipconfig();
void cmd_key();
void cmd_add_queue();
void cmd_node_list();
void cmd_stop_thread(const char * thread_name);