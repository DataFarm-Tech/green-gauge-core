#include "Packet.hpp"
#include <cbor.h>
#include <sys/socket.h>
#include "esp_log.h"
#include "coap3/coap.h"

constexpr uint8_t COAP_DEFAULT_TIME_SEC = 1;
constexpr size_t URI_BUFFER_SIZE = 40;
constexpr const char * URI = "coap://45.79.239.100/battery";

constexpr uint8_t COAP_RESPONSE_SUCCESS = 2;
constexpr uint8_t COAP_RESPONSE_CLIENT_ERROR = 4;
constexpr uint8_t COAP_RESPONSE_SERVER_ERROR = 5;


const static char * TAG = "CoAP_client";
int Packet::resp_wait = 1;
coap_optlist_t * Packet::optlist = NULL;
int Packet::wait_ms = 0;


Packet::Packet()
    : bufferLength(0) {}

const uint8_t* Packet::data() const 
{
    return buffer;
}

size_t Packet::length() const 
{
    return bufferLength;
}

coap_response_t Packet::message_handler(coap_session_t * session,
                                        const coap_pdu_t * sent,
                                        const coap_pdu_t * received, 
                                        const coap_mid_t mid)
{
    coap_pdu_code_t rcvd_code = coap_pdu_get_code(received);
    uint8_t rcvd_class = COAP_RESPONSE_CLASS(rcvd_code);

    switch (rcvd_class)
    {
        case COAP_RESPONSE_SUCCESS:
            printf("CoAP response: success (%d.%02d)\n", rcvd_class, rcvd_code & 0x1F);
            break;
        case COAP_RESPONSE_CLIENT_ERROR:
            printf("CoAP response: error (%d.%02d)\n", rcvd_class, rcvd_code & 0x1F);
            break;
        case COAP_RESPONSE_SERVER_ERROR:
            printf("CoAP response: error (%d.%02d)\n", rcvd_class, rcvd_code & 0x1F);
            break;
        
        default:
            break;
    }

    resp_wait = 0;
    return COAP_RESPONSE_OK;
}

void Packet::sendCoAP() 
{
    coap_context_t * ctx = nullptr;
    coap_session_t * session = nullptr;
    coap_pdu_t * request = nullptr;
    coap_address_t dst_addr;
    static coap_uri_t uri;
    coap_addr_info_t * info_list = nullptr;
    coap_proto_t proto;

    uint8_t uri_path[URI_BUFFER_SIZE];

    coap_startup();

    ctx = coap_new_context(NULL);
    if (!ctx) {
        ESP_LOGE(TAG, "coap_new_context() failed");
        goto clean_up;
    }

    coap_context_set_block_mode(ctx, COAP_BLOCK_USE_LIBCOAP | COAP_BLOCK_SINGLE_BODY);
    coap_register_response_handler(ctx, Packet::message_handler);


    if (coap_split_uri((const uint8_t *)URI, strlen(URI), &uri) == -1) {
        ESP_LOGE(TAG, "Invalid URI: %s", URI);
        goto clean_up;
    }

    info_list = coap_resolve_address_info(&uri.host, uri.port, uri.port, uri.port, uri.port, 0, 1 << uri.scheme, COAP_RESOLVE_TYPE_REMOTE);
    if (!info_list) {
        ESP_LOGE(TAG, "Address resolution failed");
        goto clean_up;
    }

    proto = info_list->proto;
    memcpy(&dst_addr, &info_list->addr, sizeof(dst_addr));
    coap_free_address_info(info_list);

    if (coap_uri_into_options(&uri, &dst_addr, &optlist, 1, uri_path, sizeof(uri_path)) < 0) {
        ESP_LOGE(TAG, "Failed to create URI options");
        goto clean_up;
    }

    session = coap_new_client_session(ctx, NULL, &dst_addr, proto);
    if (!session) {
        ESP_LOGE(TAG, "Failed to create session");
        goto clean_up;
    }

    request = coap_new_pdu(COAP_MESSAGE_CON, COAP_REQUEST_CODE_POST, session);
    if (!request) {
        ESP_LOGE(TAG, "Failed to create request PDU");
        goto clean_up;
    }

    coap_add_optlist_pdu(request, &optlist);

    uint8_t buf[4];

    coap_add_option(request, COAP_OPTION_CONTENT_FORMAT, coap_encode_var_safe(buf, sizeof(buf), COAP_MEDIATYPE_APPLICATION_CBOR), buf);
    coap_add_data(request, length(), data());

    resp_wait = 1;
    coap_send(session, request);

    wait_ms = COAP_DEFAULT_TIME_SEC * 1000;

    while (resp_wait) 
    {
        int result = coap_io_process(ctx, wait_ms > 1000 ? 1000 : wait_ms);
        if (result >= 0) 
        {
            if (result >= wait_ms) 
            {
                ESP_LOGE(TAG, "No response from server");
                break;
            } 
            else 
            {
                wait_ms -= result;
            }
        }
    }

    clean_up:
        if (optlist) {
            coap_delete_optlist(optlist);
            optlist = nullptr;
        }
        if (session) {
            coap_session_release(session);
        }
        if (ctx) {
            coap_free_context(ctx);
        }
        coap_cleanup();

        ESP_LOGI(TAG, "CoAP send finished");
}


bool Packet::createPayload(int temp, int humidity) {
    CborEncoder encoder, mapEncoder, arrayEncoder, batteryEncoder;

    cbor_encoder_init(&encoder, buffer, BUFFER_SIZE, 0);

    // Root map: 2 keys => "node_id" and "batteries"
    if (cbor_encoder_create_map(&encoder, &mapEncoder, 2) != CborNoError) {
        printf("CBOR: failed to create root map\n");
        return false;
    }

    // --- "node_id" : "gh6dww" ---
    if (cbor_encode_text_stringz(&mapEncoder, "node_id") != CborNoError) return false;
    if (cbor_encode_text_stringz(&mapEncoder, "nod123") != CborNoError) return false;

    // --- "batteries" : [ { "temp": 25, "humidity": 60 } ] ---
    if (cbor_encode_text_stringz(&mapEncoder, "batteries") != CborNoError) return false;

    // Start array with 1 element
    if (cbor_encoder_create_array(&mapEncoder, &arrayEncoder, 1) != CborNoError) {
        printf("CBOR: failed to create batteries array\n");
        return false;
    }

    // Each element: a map of 2 key-value pairs
    if (cbor_encoder_create_map(&arrayEncoder, &batteryEncoder, 2) != CborNoError) {
        printf("CBOR: failed to create battery object\n");
        return false;
    }

    // Add "temp": 25
    if (cbor_encode_text_stringz(&batteryEncoder, "bat_lvl") != CborNoError) return false;
    if (cbor_encode_int(&batteryEncoder, temp) != CborNoError) return false;

    // Add "humidity": 60
    if (cbor_encode_text_stringz(&batteryEncoder, "bat_hlth") != CborNoError) return false;
    if (cbor_encode_int(&batteryEncoder, humidity) != CborNoError) return false;

    // Close the inner battery map
    if (cbor_encoder_close_container(&arrayEncoder, &batteryEncoder) != CborNoError) {
        printf("CBOR: failed to close battery map\n");
        return false;
    }

    // Close the batteries array
    if (cbor_encoder_close_container(&mapEncoder, &arrayEncoder) != CborNoError) {
        printf("CBOR: failed to close batteries array\n");
        return false;
    }

    // Close root map
    if (cbor_encoder_close_container(&encoder, &mapEncoder) != CborNoError) {
        printf("CBOR: failed to close root map\n");
        return false;
    }

    // Final buffer size
    bufferLength = cbor_encoder_get_buffer_size(&encoder, buffer);
    printf("CBOR payload length: %d\n", (int)bufferLength);

    return true;
}