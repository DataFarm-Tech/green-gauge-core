#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Update.h>
#include "eeprom/eeprom.h"

#include "config.h"
#include "https_comms.h"
#include "msg_queue.h"
#include "utils.h"
#include "th/th_handler.h"

/******************* Hash Defines *****************/

/* HTTP / Controller HTTP Error Codes*/
#define HTTP_200_OK 200
#define HTTP_201_CREATED 201
#define HTTP_404_UNAUTH 404
#define HTTP_500_ERROR 500
#define HTTP_409_DUPLICATE 409

#define PING_NUM 5

/******************* Function Prototypes *****************/
int post_request(String json_payload);
void http_send(void* param);
String construct_json_payload(msg message);
void check_internet();
char* constr_endp(const char* endpoint);
void update_key(const char* new_key);
void activate_controller();

/******************* Globals *****************/
HTTPClient client;

/**
 * @brief The following function takes a new key,
 * and updates it as the new key in the EEPROM.
 */
void update_key(const char* new_key) 
{
    strncpy(config.api_key, new_key, sizeof(config.api_key) - 1);  // Avoid buffer overflow
    config.api_key[sizeof(config.api_key) - 1] = '\0';  // Ensure null-termination
    save_config();
}


/**
 * @brief The following function calls the activate_controller endpoint, it then 
 * returns an api_key. It provides a username and password in the body.
 */
void activate_controller()
{   
    char * url;
    int http_code;
    String json_payload;
    String response;
    JsonDocument doc;
    JsonDocument response_doc;
    const char* api_key;

    doc["controller_id"] = ID;
    doc["username"] = "admin";
    doc["password"] = "admin";

    serializeJson(doc, json_payload); //convert into a string

    url = constr_endp(TX_ACT_ENDPOINT);

    client.begin(url);
    client.addHeader("Content-Type", "application/json");
    http_code = client.POST(json_payload);

    switch (http_code)
    {
        case HTTP_200_OK:
            response = client.getString();
            deserializeJson(response_doc, response);
            api_key = response_doc["access_token"];

            if (api_key != nullptr) 
            {
                update_key(api_key);
            } 
            else 
            {
                PRINT_ERROR("access_token not found in response");
            }

            break;

        default:
            printf("HTTP POST failed with status code: %d\n", http_code);
            break;
    }

    client.end();
}


/******************* Function Definitions *****************/
/**
 * @brief This function is the entry point for the HTTP send thread. It sends the data to the controller API.
 * @param parameter - The parameter passed to the task
 * @return None
 */
void http_send(void* param)
{
    msg cur_msg;
    String json_payload;
    int success;

    while(1) 
    {
        // Lock the mutex before accessing the queue
        if (xSemaphoreTake(msg_queue_mh, portMAX_DELAY) == pdTRUE) 
        {
            if (!internal_msg_q.empty())
            {
                cur_msg = internal_msg_q.front();
                internal_msg_q.pop();
                xSemaphoreGive(msg_queue_mh); // Release the mutex
                json_payload = construct_json_payload(cur_msg);
                success = post_request(json_payload);
                
                while (success != HTTP_201_CREATED) 
                {
                    PRINT_INFO("POST request failed");
                    check_internet();
                    success = post_request(json_payload);
                }
            }
            else 
            {
                xSemaphoreGive(msg_queue_mh); // Release the mutex
            }

            vTaskDelay(pdMS_TO_TICKS(3000));
        }
    }

    vTaskDelete(NULL);
}

/**
 * @brief This function sends the data to the controller API using the POST method.
 * @param message - The data to be sent to the controller API
 * @return None
 */
String construct_json_payload(msg message) 
{
    // Create a JSON document
    JsonDocument doc; // Adjust size as needed
    String json_payload;
    JsonArray data;
  
    // Add node_id (src_node)
    doc["node_id"] = message.src_node;
  
    // Create the data array
    data = doc["data"].to<JsonArray>();
  
    // Helper function to add sensor data objects
    auto add_sensor_data = [&data](const char* type, float value) 
    {
        JsonObject obj = data.add<JsonObject>();
        obj["type"] = type;
        obj["level1"] = value;
        obj["level2"] = 0.0f; // Using float literal
        obj["level3"] = 0.0f;
    };
  
    // Add all sensor data
    add_sensor_data("moisture", message.data.rs485_humidity / 1.0f);
    add_sensor_data("ph", message.data.rs485_ph / 1.0f);
    add_sensor_data("temperature", message.data.rs485_temp / 1.0f);
    add_sensor_data("conductivity", message.data.rs485_con / 1.0f);
    add_sensor_data("nitrogen", message.data.rs485_nit / 1.0f);
    add_sensor_data("phosphorus", message.data.rs485_phos / 1.0f);
    add_sensor_data("potassium", message.data.rs485_pot / 1.0f);
  
    // Serialize JSON to string
    serializeJson(doc, json_payload);
    return json_payload;
  }

/**
 * @brief This function initializes the HTTP client with the given URL and adds the necessary headers.
 * @param url - The URL to which the client will connect
 * @return int - EXIT_SUCCESS on success, EXIT_FAILURE on failure
 */
int init_http_client(const String& url)
{   
    //TODO: check if url is NULL and handle it properly
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
 * @param jsonPayload - The json payload  to be sent
 * @return int - The HTTP status code of the request
 */
int post_request(String json_payload)
{
    char * url;
    int http_code;

    url = constr_endp(TX_POST_ENDPOINT); 
    
    init_http_client(url);
    http_code = client.POST(json_payload);
    client.end();

    return http_code;
}


/**
 * @brief The following function pings google.com to check internet
 * connectivity. If that succeeds then it pings the API.
 */
void check_internet() 
{
    const char* google_dns = "google.com"; // Google DNS for internet check

    // 1. Loop until WiFi connects
    while (WiFi.status() != WL_CONNECTED) 
    {
        printf("WiFi disconnected. Reconnecting...\n");
        WiFi.reconnect();
        delay(3000); // Prevents watchdog trigger
    }

    // 2. Loop until Google pings (confirms internet)
    while (1) 
    {
        printf("Pinging Google...\n");
        if (Ping.ping(google_dns, PING_NUM)) 
        {
            break;
        }

        printf("Google ping failed. Retrying in 3s...\n");
        delay(3000);
    }

    // 3. Loop until your server pings
    while (1) 
    {
        printf("Pinging API...\n");

        if (Ping.ping(API_HOST, PING_NUM)) 
        {
            // Serial.println("Server ping successful.");
            break;
        }
        printf("Server ping failed. Retrying in 3s...\n");
        delay(3000);
    }
}

/**
 * @brief Constructs the endpoint URL.
 * @param endpoint The endpoint to be appended to the URL.
 * @return char* - The constructed endpoint.
 */
char* constr_endp(const char* endpoint)
{
    static char endp[200];  // Static buffer to persist after function returns

    if (strncmp(endpoint, "", sizeof(endpoint)) == 0)
    {
        PRINT_STR("Endpoint is empty");
        return NULL;
    }
    
    snprintf(endp, sizeof(endp), "http://%s:%s%s", API_HOST, API_PORT, endpoint);
    return endp;
}