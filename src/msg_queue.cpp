#include "msg_queue.h"

std::queue<msg> internal_msg_q;  // Define the msg queue here

void add_msg_to_queue() {
    // Create a new message
    msg new_message;
    new_message.src_node = "tnode1";
    new_message.des_node = "controller";
    
    // Generate some sensor data (you can replace with real sensor readings)
    new_message.data = {
        .rs485_humidity = uint8_t(random(30, 60)),
        .rs485_temp = uint8_t(random(20, 30)),
        .rs485_con = uint8_t(random(20, 40)),
        .rs485_ph = uint8_t(random(5, 9)),
        .rs485_nit = uint8_t(random(40, 60)),
        .rs485_phos = uint8_t(random(30, 50)),
        .rs485_pot = uint8_t(random(25, 45))
    };
        
    // Lock the mutex before accessing the queue
    if (xSemaphoreTake(msg_queue_mh, portMAX_DELAY) == pdTRUE) {
        // Add the new message to the queue
        internal_msg_q.push(new_message);
        xSemaphoreGive(msg_queue_mh); // Release the mutex
    }
    
    Serial.printf("Produced message from %s\n", new_message.src_node.c_str());
}