#include "msg_queue.h"

std::queue<msg> internal_msg_q;  // Define the msg queue here

char** node_list;     // define Array of node IDs (strings)
size_t node_count; // define Number of nodes in the list