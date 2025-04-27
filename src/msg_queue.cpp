#include "msg_queue.h"

std::queue<msg> internal_msg_q;  // Define the msg queue here
std::mutex queue_mutex; // Define the mutex here
