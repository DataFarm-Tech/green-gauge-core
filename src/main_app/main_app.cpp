#include "main_app/main_app.h"
#include "utils.h"
#include <Arduino.h>
#include <WiFi.h>
#include "th/th_handler.h"
#include "interrupts.h"
#include "msg_queue.h"
#include "lora/lora_listener.h"
#include "pack_def/pack_def.h"


#define CONTROLLER_INTERVAL_SEC 28800 //change me to 60 for testing

bool exec_flag = false;
uint32_t last_run_time = 0;  // Store the last execution time in epoch format
const char * src_node_id =  ID;
uint8_t seq_id = 0;

void app();

void main_app(void *parm)
{
    //     /**
    //      * This portion of code doesnt make any sense
    //      * But it still works??
    //      * Calling the delete_th function removes it, but doesnt 
    //      * show the removal in the threads cmd.
    //      * Will find a work around.
    //      */
    //     main_app_th = NULL;
    //     vTaskDelete(NULL);
    // } 

    while (1)
    {
        if (!hourly_timer_flag)
        {
            printf("Not yet an hour\n");
            app();
        }
        else
        {
            printf("An hour has passed\n");
            hourly_timer_flag = false; // Reset the flag
            app();
        }
        
        sleep(2);
    }
}

void app()
{
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
            
            for (int j = 0; j < PACKET_LENGTH; j++)
            {
                printf("%02x ", packet_to_send[j]);
            }
            printf("\n");

            send_packet(packet_to_send, sizeof(packet_to_send));
            seq_id++;
        }
    }
}