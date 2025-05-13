#include "msg_queue.h"

std::queue<sn001_suc_rsp> internal_msg_q;  // Define the msg queue here

char** node_list;     // define Array of node IDs (strings)
size_t node_count; // define Number of nodes in the list
uint8_t ttl;
int hash_cache_size;
uint8_t seq_id = 0;