#pragma once

#include <queue>
#include <mutex>
#include <Arduino.h>
#include "config.h"
#include "mh/mutex_h.h"

typedef struct {
    uint8_t rs485_humidity;
    uint8_t rs485_temp;
    uint8_t rs485_con;
    uint8_t rs485_ph;
    uint8_t rs485_nit;
    uint8_t rs485_phos;
    uint8_t rs485_pot;
} rs485_data;

typedef struct {
    String src_node;
    String des_node;
    rs485_data data;    
} msg;

extern std::queue<msg> internal_msg_q;  // declare the queue here

extern char** node_list;     // Array of node IDs (strings)
extern size_t node_count; // Number of nodes in the list
