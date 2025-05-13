#pragma once

#include <queue>
#include <mutex>
#include <Arduino.h>
#include "config.h"
#include "mutex_h.h"

typedef struct {
    uint8_t rs485_humidity;
    uint8_t rs485_temp;
    uint8_t rs485_con;
    uint8_t rs485_ph;
    uint8_t rs485_nit;
    uint8_t rs485_phos;
    uint8_t rs485_pot;
} rs485_data;

typedef struct
{
    String src_node;
    String des_node;
    rs485_data data;
} sn001_suc_rsp;


extern std::queue<sn001_suc_rsp> internal_msg_q;

extern char** node_list;     // Array of node IDs (strings)
extern size_t node_count; // Number of nodes in the list
extern uint8_t ttl;
extern int hash_cache_size;
extern volatile bool hourlyTimerFlag;
extern uint8_t seq_id;