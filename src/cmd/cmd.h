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
    CMD_TEARDOWN,
    CMD_IPCONFIG,
    CMD_QADD,
    CMD_APPLY,
    CMD_KEY,
    CMD_CLEAR_CONFIG,
    CMD_LIST,
    CMD_CACHE,
    CMD_STOP_THREAD,
    CMD_START_THREAD,
    CMD_STATE,
    CMD_DISCONNECT_WIFI,
    CMD_CONNECT_WIFI,
    CMD_UNKNOWN
} cli_cmd;

void cmd_help();
void cmd_exit();
void cmd_reboot();
void cmd_queue();
void cmd_ping(const char* host);
void cmd_clear();
void cmd_threads();
void cmd_teardown();
void cmd_ipconfig();
void cmd_key();
void cmd_add_queue(const char * src_node, const char * des_node);
void cmd_node_list();
void cmd_cache();
void cmd_check_state();
void cmd_stop_thread(const char * thread_name);
void cmd_start_thread(const char * thread_name);
void cmd_disconnect_wifi(const char * arg);
void cmd_connect_wifi();