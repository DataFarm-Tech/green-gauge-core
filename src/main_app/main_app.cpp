#include <Arduino.h>

#include "main_app/main_app.h"
#include "utils.h"
#include "th/th_handler.h"
#include "interrupts.h"
#include "msg_queue.h"
#include "lora/lora_listener.h"
#include "pack_def/pack_def.h"

uint8_t seq_id = 0;

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

    packet pkt;
    uint8_t packet_to_send[PACKET_LENGTH];

    if (node_count > 0)
    {
        for (int i = 0; i < node_count; i++)
        {
            memset(packet_to_send, 0, sizeof(packet_to_send)); // Clear the packet buffer
            memset(&pkt, 0, sizeof(pkt)); // Clear the packet structure
            
            strcpy(pkt.src_node, ID); //copy the source node ID
            strcpy(pkt.des_node, node_list[i]); //copy the destination node ID
            pkt.ttl = ttl; // Set the time-to-live
            pkt.num_nodes = node_count; //  Set the number of nodes
            memset(pkt.data, 0, sizeof(pkt.data)); // Clear data field

            create_packet(packet_to_send, &pkt, seq_id);
            
            // for (int j = 0; j < PACKET_LENGTH; j++)
            // {
            //     printf("%02x ", packet_to_send[j]);
            // }
            // printf("\n");

            send_packet(packet_to_send, sizeof(packet_to_send));
            seq_id++;
        }
    }
}