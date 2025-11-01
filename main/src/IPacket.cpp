#include <esp_log.h>
#include <cstring>

#include <cbor.h>
#include <sys/socket.h>
#include "esp_log.h"
#include "coap3/coap.h"
#include <cstdint>
#include <cstddef>
#include "IPacket.hpp"

#define URI "coap://192.168.1.100/sensor"
#define URI_BUFFER_SIZE 128

static const char* TAG = "IPacket";
static int resp_wait = 0;
static int wait_ms = 1000;

constexpr uint8_t COAP_RESPONSE_SUCCESS = 2;
constexpr uint8_t COAP_RESPONSE_CLIENT_ERROR = 4;
constexpr uint8_t COAP_RESPONSE_SERVER_ERROR = 5;


coap_response_t IPacket::message_handler(coap_session_t * session, const coap_pdu_t * sent,
                                         const coap_pdu_t * received, const coap_mid_t mid) {
    coap_pdu_code_t rcvd_code = coap_pdu_get_code(received);
    uint8_t rcvd_class = COAP_RESPONSE_CLASS(rcvd_code);

    switch (rcvd_class) {
        case COAP_RESPONSE_SUCCESS:
            ESP_LOGI(TAG, "CoAP response success (%d.%02d)", rcvd_class, rcvd_code & 0x1F);
            break;
        case COAP_RESPONSE_CLIENT_ERROR:
        case COAP_RESPONSE_SERVER_ERROR:
            ESP_LOGW(TAG, "CoAP response error (%d.%02d)", rcvd_class, rcvd_code & 0x1F);
            break;
        default:
            break;
    }

    resp_wait = 0;
    return COAP_RESPONSE_OK;
}

void IPacket::sendPacket() {
    uint8_t * buffer = toBuffer();
    if (!buffer) {
        ESP_LOGE(TAG, "Failed to get buffer");
        return;
    }

    coap_startup();
    coap_context_t* ctx = coap_new_context(NULL);
    if (!ctx) {
        ESP_LOGE(TAG, "Failed to create CoAP context");
        return;
    }

    coap_context_set_block_mode(ctx, COAP_BLOCK_USE_LIBCOAP | COAP_BLOCK_SINGLE_BODY);
    coap_register_response_handler(ctx, IPacket::message_handler);

    coap_session_t* session = nullptr;
    coap_address_t dst_addr;
    coap_uri_t uri;
    if (coap_split_uri((const uint8_t*)URI, strlen(URI), &uri) == -1) {
        ESP_LOGE(TAG, "Invalid URI: %s", URI);
        coap_free_context(ctx);
        coap_cleanup();
        return;
    }

    session = coap_new_client_session(ctx, NULL, &dst_addr, COAP_PROTO_UDP);
    if (!session) {
        ESP_LOGE(TAG, "Failed to create CoAP session");
        coap_free_context(ctx);
        coap_cleanup();
        return;
    }

    coap_pdu_t* request = coap_new_pdu(COAP_MESSAGE_CON, COAP_REQUEST_CODE_POST, session);
    if (!request) {
        ESP_LOGE(TAG, "Failed to create request PDU");
        coap_session_release(session);
        coap_free_context(ctx);
        coap_cleanup();
        return;
    }

    uint8_t buf[4];
    coap_add_option(request, COAP_OPTION_CONTENT_FORMAT,
                    coap_encode_var_safe(buf, sizeof(buf), COAP_MEDIATYPE_APPLICATION_CBOR),
                    buf);
    coap_add_data(request, this->bufferLength, buffer);

    resp_wait = 1;
    coap_send(session, request);

    while (resp_wait) {
        int result = coap_io_process(ctx, wait_ms > 1000 ? 1000 : wait_ms);
        if (result >= 0) {
            if (result >= wait_ms) {
                ESP_LOGW(TAG, "No response from server");
                break;
            } else {
                wait_ms -= result;
            }
        }
    }

    coap_session_release(session);
    coap_free_context(ctx);
    coap_cleanup();
    ESP_LOGI(TAG, "CoAP send finished");
}
