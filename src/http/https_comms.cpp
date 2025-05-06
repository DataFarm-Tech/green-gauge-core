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
#include "interrupts.h"

/******************* Hash Defines *****************/

/* HTTP / Controller HTTP Error Codes*/
#define HTTP_200_OK 200
#define HTTP_201_CREATED 201
#define HTTP_401_UNAUTH 401
#define HTTP_404_NOTFOUND 404
#define HTTP_500_ERROR 500
#define HTTP_409_DUPLICATE 409

#define PING_NUM 5

/******************* Function Prototypes *****************/

typedef enum
{
    POST,
    GET
} request_type;

int post_request(String json_payload);
String construct_json_payload(message message);
void check_internet();
char* constr_endp(const char* endpoint);
void update_key(const char* new_key);
int init_http_client(const String& url, request_type type);

/******************* Globals *****************/
HTTPClient client;

/******************* Function Definitions *****************/

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

    url = constr_endp(TX_ACT_ENDPOINT);

    if (init_http_client(url, POST) == EXIT_FAILURE)
    {
        PRINT_ERROR("Unable to initialise key"); //TODO
    }

    doc["controller_id"] = ID;
    doc["username"] = "admin";
    doc["password"] = "admin";

    serializeJson(doc, json_payload); //convert into a string
    http_code = client.POST(json_payload);

    while (http_code != HTTP_200_OK) 
    {
        switch (http_code)
        {
            case HTTP_401_UNAUTH:
                printf("[%d]: Unauthorized access. Key is invalid.\n", http_code);
                break;
            default:
                break;
        }

        check_internet();
        http_code = client.POST(json_payload);
    }

    response = client.getString();
    client.end();

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
}

/**
 * @brief The following function calls the /user/get/node endpoint to retrieve 
 *        the list of node IDs associated with the controller.
 * TODO: Add the api_key header before running the GET request.
 * TODO: Create a global list in the config variable, that holds the node list and the node count.
 * TODO: Change this to a void instead of bool. In main app we will check if node_list is NULL. If it is then we delete the main_app thread.
 */
void get_nodes_list() 
{
    char url[150];
    String response;
    JsonDocument doc;
    int success;
    
    snprintf(url, sizeof(url), "%s?controller_id=%s", constr_endp(TX_GET_ENDPOINT), ID);

    if (init_http_client(url, GET) == EXIT_FAILURE)
    {
        PRINT_ERROR("Unable to initialise key");
    }
    
    success = client.GET();
                
    while (success != HTTP_200_OK) 
    {
        switch (success)
        {
            case HTTP_401_UNAUTH:
                printf("[%d]: Unauthorized access. Key is invalid.\n", success);
                break;
            case HTTP_404_NOTFOUND:
                printf("[%d]: %s is not a valid controller\n", success, ID);
                break;
            default:
                break;
        }

        check_internet();
        success = client.GET();
    }
    
    response = client.getString();
    client.end();
    
    deserializeJson(doc, response);


    node_count = doc.size();
    node_list = (char**)malloc(node_count * sizeof(char*));
    
    if (node_list != nullptr) 
    {
        for (int i = 0; i < node_count; i++) 
        {
            const char* node_id = doc[i]["node_id"];
            (node_list)[i] = strdup(node_id);
            
            if ((node_list)[i] == nullptr)  /* This should never happen*/
            {
                DEBUG();
                PRINT_ERROR("String duplication failed");
                // Free any allocated memory before returning
                for (int j = 0; j < i; j++) 
                {
                    free((node_list)[j]);
                }
    
                free(node_list);
                return;
            }
        }
    
        ttl = (2 * node_count) + 1;
        hash_cache_size = MIN(10, node_count);
    }
    else
    {
        DEBUG();
        PRINT_ERROR("Memory allocation failed");
        return;
    }

}



/**
 * @brief This function is the entry point for the HTTP send thread. It sends the data to the controller API.
 * @param parameter - The parameter passed to the task
 * @return None
 */
void http_send(void* param)
{
    message cur_msg;
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
                    switch (success)
                    {
                        case HTTP_401_UNAUTH:
                            printf("[%d]: Unauthorized access. Key is invalid.\n", success);
                            break;
                        case HTTP_404_NOTFOUND:
                            printf("[%d]: Node %s does not exist...\n", success, cur_msg.src_node.c_str());
                            break;
                        
                    default:
                        break;
                    }
                    
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
String construct_json_payload(message message) 
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
int init_http_client(const String& url, request_type type)
{
    client.begin(url);
    client.addHeader("access_token", config.api_key);
    
    switch (type)
    {
        case POST:
            client.addHeader("Content-Type", "application/json");
            break;
        case GET:
            client.addHeader("accept", "application/json");
            break;
    
    default:
        break;
    }

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
    
    if (init_http_client(url, POST) == EXIT_FAILURE)
    {
        return EXIT_FAILURE;
    }
    
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

    if (strncmp(endpoint, "", strlen(endpoint)) == 0)
    {
        PRINT_STR("Endpoint is empty");
        return NULL;
    }
    
    snprintf(endp, sizeof(endp), "http://%s:%s%s", API_HOST, API_PORT, endpoint);
    return endp;
}