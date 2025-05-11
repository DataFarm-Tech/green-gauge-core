#include <Arduino.h>

#include "main_app/main_app.h"
#include "utils.h"
#include "th/th_handler.h"
#include "interrupts.h"
#include "msg_queue.h"
#include "lora/lora_listener.h"
#include "pack_def/pack_def.h"
#include "msg_queue.h"

void app();

void main_app(void *parm)
{
    app();

    while (1)
    {
        if (hourly_timer_flag)
        {
            hourly_timer_flag = false; // Reset the flag
            app();
        }
        
        sleep(2);
    }
}

void app()
{
    PRINT_INFO("Starting app...\n");

    cn001_req req_pkt;
    uint8_t packet_to_send[CN001_REQ_LEN];

    if (node_count > 0)
    {
        for (int i = 0; i < node_count; i++)
        {
            memset(packet_to_send, 0, sizeof(packet_to_send)); // Clear the packet buffer
            memset(&req_pkt, 0, sizeof(req_pkt)); // Clear the packet structure
            
            strcpy(req_pkt.src_node, ID); //copy the source node ID
            strcpy(req_pkt.des_node, node_list[i]); //copy the destination node ID
            
            req_pkt.ttl = ttl; // Set the time-to-live
            req_pkt.num_nodes = node_count; //  Set the number of nodes
            
            pkt_cn001_req(packet_to_send, &req_pkt, seq_id); // Create the packet

            send_packet(packet_to_send, sizeof(packet_to_send));

            if (xSemaphoreTake(seq_mh, portMAX_DELAY) == pdTRUE)
            {
                seq_id++;
                xSemaphoreGive(seq_mh);
            }
        }
    }
}