#include <Arduino.h>
#include <string.h>

#include "main_app.h"
#include "utils.h"
#include "th_handler.h"
#include "interrupts.h"
#include "msg_queue.h"
#include "lora_listener.h"
#include "pack_def.h"
#include "msg_queue.h"

void app();

#define RESPONSE_TIMEOUT 120000 // 2 minutes in milliseconds

void main_app(void * param)
{
    app();

    while (1)
    {
        if (hourly_timer_flag)
        {
            hourly_timer_flag = false;
            app();
        }

        sleep(2);
    }
}


// Wait for a matching response from internal_msg_q using snapshot checking with timeout
bool wait_for_response(const char *dest_node)
{
    unsigned long start_time = millis();

    while ((millis() - start_time) < RESPONSE_TIMEOUT)
    {
        std::queue<sn001_suc_rsp> tmp_queue = internal_msg_q; // Copy queue snapshot each time

        while (!tmp_queue.empty())
        {
            sn001_suc_rsp msg = tmp_queue.front();
            tmp_queue.pop();

            if (msg.des_node.equals(String(ID)) && msg.src_node.equals(String(dest_node)))
            {
                return true; // Matched response found
            }
        }

        delay(100); // Avoid tight CPU loop
    }

    return false; // Timeout with no match
}


void app()
{
    PRINT_INFO("Starting app...\n");

    cn001_req req_pkt;

    if (node_count > 0)
    {
        uint8_t packet_to_send[CN001_REQ_LEN];

        for (int i = 0; i < node_count; i++)
        {
            memset(packet_to_send, 0, sizeof(packet_to_send));
            memset(&req_pkt, 0, sizeof(req_pkt));

            strcpy(req_pkt.src_node, ID);
            strcpy(req_pkt.des_node, node_list[i]);

            req_pkt.ttl = ttl;
            req_pkt.num_nodes = node_count;

            pkt_cn001_req(packet_to_send, &req_pkt, seq_id);

            if (send_packet(packet_to_send, sizeof(packet_to_send)) == EXIT_SUCCESS)
            {
                if (xSemaphoreTake(seq_mh, portMAX_DELAY) == pdTRUE)
                {
                    seq_id++;
                    xSemaphoreGive(seq_mh);
                }

                printf("sent packet\n");

                if (wait_for_response(req_pkt.des_node))
                {
                    // PRINT_INFO("Received response from %s.\n", req_pkt.des_node);
                    printf("rec response\n");
                }
                else
                {
                    printf("no response\n");
                    /** Notify user for error */
                }
            }
        }
    }
}
