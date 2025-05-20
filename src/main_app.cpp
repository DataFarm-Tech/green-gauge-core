#include <Arduino.h>
#include <string.h>
#include "https_comms.h"
#include "main_app.h"
#include "utils.h"
#include "th_handler.h"
#include "interrupts.h"
#include "msg_queue.h"
#include "lora_listener.h"
#include "pack_def.h"
#include "msg_queue.h"

#define RESPONSE_TIMEOUT 120000 // 2 minutes in milliseconds

void app();

/**
 * @brief The following function is the main_app thread.
 * This thread calls the app() function every X hours. It uses the HW timer
 * to ensure it is executed correctly.
 */
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


/**
 * @brief The following function waits until @param dest_node
 * appears in a snapshot queue.
 * @return bool
 */
bool wait_for_response(const char *dest_node)
{
    uint32_t start_time = millis();

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

/**
 * @brief The following function starts t
 */
void app()
{
    PRINT_INFO("Starting app...");

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

                if (wait_for_response(req_pkt.des_node))
                {
                    printf("[INFO]: Recieved Response from %s\n", req_pkt.des_node);
                }
                else
                {
                    printf("[INFO]: No Response from %s\n", req_pkt.des_node);
                    cn001_notify_message(req_pkt.des_node, SN001_ERR_RSP_CODE_B);

                    /** Notify user for error */
                }
            }
        }
    }
}
