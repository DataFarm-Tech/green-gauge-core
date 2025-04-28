#include "main_app/main_app.h"
#include "utils.h"
#include "ntp/ntp.h"
#include <Arduino.h>
#include <WiFi.h>
#include "th/th_handler.h"
#include "interrupts.h"
#define CONTROLLER_INTERVAL_SEC 28800 //change me to 60 for testing

bool exec_flag = false;
uint32_t last_run_time = 0;  // Store the last execution time in epoch format

void main_app(void *parm)
{
    uint32_t currentTime;

    if (!start_sys_time() || !get_sys_time(&currentTime)) 
    {
        PRINT_ERROR("Unable to interface sys time killing main_app thread!");
        
        /**
         * This portion of code doesnt make any sense
         * But it still works??
         * Calling the delete_th function removes it, but doesnt 
         * show the removal in the threads cmd.
         * Will find a work around.
         */
        main_app_th = NULL;
        vTaskDelete(NULL);
    } 

    while (1)
    {
        if (!exec_flag)
        {
            exec_flag = true;
            PRINT_WARNING("Starting APP MAIN");
            //call function
            last_run_time = currentTime;  // Set the initial time for the first execution
        }
        else
        {
            // Check if 6 hours (21600 seconds) have passed
            if (currentTime - last_run_time >= CONTROLLER_INTERVAL_SEC)  // 6 hours = 6 * 60 * 60 = 21600 seconds
            { 
                PRINT_WARNING("Starting APP MAIN");
                //call function
                last_run_time = currentTime;  // Update the last execution time
            }
        }
        
        sleep(1);
    }
}


// #include "config.h"
// #include "crc.h"
// #include "types.h"
// #include "https_comms.h"
// #include "rx_lora_listener.h"
// #include "queue_m.h"
// #include "wlan_connect.h"
// #include "utils.h"

// #include <queue>
// #include <iostream>
// #include <vector>
// #include <string>            
// #include <Arduino.h>         // Basic lib for int, etc
// #include <NTPClient.h>       // NTPClient library for getting time
// #include <WiFiUdp.h>         // UDP library for communication

// /******************* Hash Defines *****************/
// #define TIMEZONE_OFFSET 0 //UTC timezone
// #define UPDATE_INTERVAL 60000
// #define NTP_SERVER_POOL "pool.ntp.org"
// #define TIMEOUT_MS 60000

// /******************* Function Prototypes *****************/
// int main(void);
// void setup(void);
// void loop(void);
// bool check_for_packet(String destNodeId);

// /******************* Globals *****************/
// TaskHandle_t lora_listener_th; //thread handler for lora listen thread
// TaskHandle_t http_send_th; //thread handler for http send thread
// WiFiUDP udp;
// NTPClient time_client(udp, NTP_SERVER_POOL, TIMEZONE_OFFSET, UPDATE_INTERVAL);  // define and init ntp client
// std::queue<msg> internal_msg_q;  // Define the msg queue here
// bool exec_flag = false;
// uint32_t last_run_time = 0;  // Store the last execution time in epoch format
// String src_node_id = CONTROLLER_ID; // Assuming CONTROLLER_ID is a string (6 chars long)

// /******************* Function Definitions *****************/

// /**
//  * 
//  * @brief This function checks if a packet with the destination node ID is present in the internal message queue.
//  * @param dest_node_id - The destination node ID to check for in the queue
//  * @return bool - true if the packet is found, false otherwise
//  */
// bool check_for_packet(String dest_node_id) 
// {
//     bool found = false; //flag to track if node is found in the queue
//     std::queue<msg> tmp_queue = internal_msg_q;

//     // Loop through the queue to see if any packet has src_node matching the destNodeId
//     while (!tmp_queue.empty()) 
//     {
//         msg cur_msg = tmp_queue.front();
//         tmp_queue.pop();

//         // Check if the src_node matches the destNodeId
//         if (cur_msg.src_node == dest_node_id) 
//         {
//             found = true;
//             break;
//         }
//     }

//     return found;
// }

// /**
//  * @brief This function is the entry point of the program. It initializes the serial console, LoRa module,
//  * Wifi connection, and spawns the LoRa listener and HTTP sender threads. It also starts the NTP time tracking.
//  * @param None
//  * @return None
//  */
// void setup(void)
// {
//     Serial.begin(BAUD_RATE); //start serial console
//     sleep(5); //wait until serial console is opened

//     get_sys_info(); //print sys info

//     setup_lora(); //set pins/freq for lora rx/tx

//     connect_wlan(); //connect to Wifi via wifi manager

    
//     #ifndef OTA_ENABLE
//         PRINT_STR("OTA is disabled");
//     #else
//         PRINT_STR("OTA is enabled");
//         // Call system update here!!
//         exec_update(); // Check for updates
//     #endif

    
//     xTaskCreatePinnedToCore(lora_listener, "lora_listener", 10000, NULL, 1, &lora_listener_th, 0); //create lora listen thread
//     xTaskCreatePinnedToCore(http_send, "http_send", 10000, NULL, 1, &http_send_th, 1); //create http thread

//     time_client.begin();  // Retrieve time from NTP
//     last_run_time = time_client.getEpochTime();  // Set initial run time to the current time
// }
 

// /**
//  * 
//  * @brief This function is the main loop of the program. It checks if the main function has been executed before.
//  * If not, it calls the main function. If it has, it checks if 6 hours have passed since the last execution.
//  * If so, it calls the main function again.
//  * @param None
//  * @return None
//  */
// void loop(void)
// {
//     time_client.update(); //update the time every iteration
//     uint32_t currentTime = time_client.getEpochTime();  // Get current time in seconds
    
//     if (!exec_flag)
//     {
//         exec_flag = true;
//         printf("Starting main...\n");
//         main();
//         last_run_time = currentTime;  // Set the initial time for the first execution
//     }
//     else
//     {
//         // Check if 6 hours (21600 seconds) have passed
//         if (currentTime - last_run_time >= CONTROLLER_INTERVAL_SEC)  // 6 hours = 6 * 60 * 60 = 21600 seconds
//         { 
//             printf("Starting main...\n");
//             main();
//             last_run_time = currentTime;  // Update the last execution time
//         }
//     }
// }

// /**
//  * @brief This function gets nodes from the API, then sends a LoRa packet for each node.
//  * It then waits for the node to respond with a packet. If it matches, then it is sent to the 
//  * internal message queue, where the http_send thread reads and sends an HTTP request to Net-Link.
//  * 
//  * @param None
//  * @return int - EXIT_SUCCESS on success, EXIT_FAILURE on failure
//  */
// int main(void)
// {
//     String node_ids[MAX_NODES_LIST];
//     String dest_node_id;
//     int node_count = 0;
//     int j;
//     uint32_t timeout;
//     bool packet_recv = false;
    
//     PRINT_STR("Starting main...");

//     // Call the function to get node IDs from API
//     get_node_list(node_ids, node_count);

//     if (node_count > 0)
//     { 
//         for (int i = 0; i < node_count; i++) 
//         {
//             // Initialize the LoRa packet with a size that can hold the node IDs and additional data
//             uint8_t packet[LORA_MSG_LEN];  // Assuming each packet will hold 6 bytes for nodeId + 6 bytes for source nodeId
            
//             // Fill in the packet: Write the nodeId[i] as the destination node address
//             dest_node_id = node_ids[i];

//             // Convert the nodeId to a byte array (6 bytes)
//             for (j = 0; j < ADDRESS_SIZE; j++) 
//             {
//                 packet[j] = dest_node_id[j]; // Store each character as a byte
//             }

//             // Write the CONTROLLER_ID (also a 6-character ID) as the source node address
//             for (j = 0; j < ADDRESS_SIZE; j++) 
//             {
//                 packet[ADDRESS_SIZE + j] = src_node_id[j];  // Store each character as a byte in the next 6 bytes
//             }

//             //write empty values for data.
//             for (j = ADDRESS_SIZE * 2; j < LORA_MSG_LEN - 2; j++)
//             {
//                 packet[j] = 0;
//             }
            
//             calc_crc(packet, LORA_MSG_LEN - 2);
//             send_packet(packet, sizeof(packet));

//             // Now wait for the response in the internal_msg_q queue
//             timeout = millis() + TIMEOUT_MS;  // Timeout after 2 minutes

//             for (int z = 0; z < sizeof(packet); z++)
//             {
//                 printf("%c ", packet[z]);
//             }
        
//             while (millis() < timeout) 
//             {
//                 if (check_for_packet(dest_node_id)) 
//                 {
//                     packet_recv = true;
//                     printf("Received response for %s, continuing...\n", dest_node_id);
//                     break;
//                 }
//                 delay(100); // Small delay to prevent the loop from consuming too much CPU time
//             }

//             if (!packet_recv) 
//             {
//                 printf("Timeout reached for node %s, moving to next node.\n", dest_node_id);
//             }
//         }

//         return EXIT_SUCCESS;
//     } 
//     else 
//     {
//         PRINT_STR("No available nodes for this controller.");
//         return EXIT_FAILURE;
//     }
//     delay(1000);
// }