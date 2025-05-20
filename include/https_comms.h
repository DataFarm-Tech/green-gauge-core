#pragma once

#include <Arduino.h>

void http_send(void* param);
void activate_controller();
void get_nodes_list();
void cn001_notify_message(String src_node, uint8_t code);


