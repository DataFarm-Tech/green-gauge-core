#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <Update.h>

#include "config.h"
#include "https_comms.h"
#include "msg_queue.h"
#include "utils.h"
#include "th/th_handler.h"

/******************* Hash Defines *****************/

/* HTTP / Controller HTTP Error Codes*/
#define HTTP_200_OK 200
#define HTTP_201_CREATED 201
#define HOST_FAILED_PING -1
#define RSSI_LOW_STRENGTH -2

/******************* Function Prototypes *****************/
int post_request(const String& type, uint8_t data);
void http_send(void* parameter);
void send_data(rs485_data data);

/******************* Globals *****************/
HTTPClient client;
String node_id;

const char* labels[] = {
    "moisture", 
    "temperature", 
    "conductivity", 
    "nitrogen", 
    "potassium", 
    "phosphorus", 
    "ph"
};

/******************* Function Definitions *****************/
/**
 * @brief This function is the entry point for the HTTP send thread. It sends the data to the controller API.
 * @param parameter - The parameter passed to the task
 * @return None
 */
void http_send(void* parameter)
{
    /**
     * This portion of code doesnt make any sense
     * But it still works??
     * Calling the delete_th function removes it, but doesnt 
     * show the removal in the threads cmd.
     * Will find a work around.
     */
    http_th = NULL;
    vTaskDelete(NULL);

    msg cur_msg;

    while(1) 
    {
        // Lock the mutex before accessing the queue
        queue_mutex.lock();

        if (!internal_msg_q.empty())
        {
            cur_msg = internal_msg_q.front();
            internal_msg_q.pop();
            // Unlock the mutex after accessing the queue
            queue_mutex.unlock();

            node_id = cur_msg.src_node;
            send_data(cur_msg.data);
        }
        else 
        {
            // If queue was empty, don't forget to unlock!
            queue_mutex.unlock();
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    vTaskDelete(NULL);
}

/**
 * @brief This function sends the data to the controller API using the POST method.
 * @param data - The data to be sent to the controller API
 * @return None
 */
void send_data(rs485_data data)
{
    // Array of labels and corresponding data values
    int data_values[] = {data.rs485_humidity, data.rs485_temp, data.rs485_con, data.rs485_nit, data.rs485_pot, data.rs485_phos, data.rs485_ph};
    int success = -1; // Initialize success variable to -1

    // Loop through the labels and data values
    for (int i = 0; i < sizeof(data_values); i++) 
    {

        success = post_request(labels[i], data_values[i]);

        while (success == -1 || success == -2)
        {
            PRINT_STR("POST request failed");
            success = post_request(labels[i], data_values[i]);
        }
        PRINT_STR("POST request sent");
        
        delay(1000);
    }
}


/**
 * @brief This function initializes the HTTP client with the given URL and adds the necessary headers.
 * @param url - The URL to which the client will connect
 * @return int - EXIT_SUCCESS on success, EXIT_FAILURE on failure
 */
int init_http_client(const String& url)
{
    if (strncmp(API_KEY, "", sizeof(API_KEY)) == 0)
    {
        PRINT_STR("API key not initialized");
        DEBUG();
        return EXIT_FAILURE; //TODO: replace to hash define exit  failure
    }
    
    client.begin(url);
    client.addHeader("Content-Type", "application/json");
    client.addHeader("access_token", API_KEY);

    return EXIT_SUCCESS;
}


/**
 * @brief This function sends a POST request to the controller API with the given type and data.
 * @param type - The type of data being sent
 * @param data - The data value to be sent
 * @return int - The HTTP status code of the request
 */
int post_request(const String& type, uint8_t data)
{
    int http_code;
    String json_payload;
    
    init_http_client("badendpoint"); //Change this to the actual endpoint

    json_payload = "{\"node_id\":\"" + node_id + "\","
                          "\"level1\":\"" + data + "\","
                          "\"level2\":\"" + data + "\","
                          "\"level3\":\"" + data + "\","
                          "\"type\":\"" + type + "\"}";

    // Send the POST request with the JSON payload & close client connection.
    http_code = client.POST(json_payload);
    client.end();

    return http_code;
}
