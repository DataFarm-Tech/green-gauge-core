#include <esp_log.h>
#include <cstring>

#include <cbor.h>
#include <sys/socket.h>
#include "esp_log.h"
#include "coap3/coap.h"
#include <cstdint>
#include <cstddef>
#include "IPacket.hpp"
#include "Config.hpp"

static const char * TAG = "IPacket";
static bool resp_wait = false;


coap_response_t IPacket::message_handler(coap_session_t * session, const coap_pdu_t * sent,
                                         const coap_pdu_t * received, const coap_mid_t mid) {
    coap_pdu_code_t rcvd_code = coap_pdu_get_code(received);
    CoAPResponseType rcvd_class = static_cast<CoAPResponseType>(COAP_RESPONSE_CLASS(rcvd_code));

    switch (rcvd_class) {
        case CoAPResponseType::COAP_RESPONSE:
            ESP_LOGI(TAG, "CoAP response success (%d.%02d)", rcvd_class, rcvd_code & 0x1F);
            break;
        case CoAPResponseType::COAP_RESPONSE_CLIENT_ERROR:
        case CoAPResponseType::COAP_RESPONSE_SERVER_ERROR:
            ESP_LOGW(TAG, "CoAP response error (%d.%02d)", rcvd_class, rcvd_code & 0x1F);
            break;
        default:
            break;
    }

    resp_wait = false;
    return COAP_RESPONSE_OK;
}

void IPacket::sendPacket() {
    const uint8_t * buf_ptr = toBuffer();

    // Better debug output
    if (buf_ptr == nullptr || bufferLength == 0) {
        ESP_LOGE(TAG, "Packet buffer is null or empty — aborting send");
        return;
    }
    
    ESP_LOGI(TAG, "Preparing to send %zu bytes", bufferLength);
    // Fix type conversion warning
    uint16_t log_len = (bufferLength > 32) ? 32 : static_cast<uint16_t>(bufferLength);
    ESP_LOG_BUFFER_HEX_LEVEL(TAG, buf_ptr, log_len, ESP_LOG_DEBUG);

    bool use_dtls;
    coap_context_t * ctx = nullptr;
    coap_optlist_t * optlist = nullptr;
    coap_session_t * session = nullptr;
    coap_pdu_t * request = nullptr;
    coap_address_t dst_addr;
    static coap_uri_t uri_t;
    coap_addr_info_t * info_list = nullptr;
    coap_proto_t proto;
    int total_wait_ms = 5000;  // 5 second timeout
    int elapsed_ms = 0;
    uint8_t uri_path[GEN_BUFFER_SIZE];
    coap_mid_t mid = COAP_INVALID_MID;  // Declare here to avoid goto issues

    coap_startup();

    ctx = coap_new_context(NULL);
    if (!ctx) {
        ESP_LOGE(TAG, "coap_new_context() failed");
        goto clean_up;
    }

    coap_context_set_block_mode(ctx, COAP_BLOCK_USE_LIBCOAP | COAP_BLOCK_SINGLE_BODY);
    coap_register_response_handler(ctx, IPacket::message_handler);

    if (coap_split_uri((const uint8_t *)this->uri.c_str(), strlen(this->uri.c_str()), &uri_t) == -1) {
        ESP_LOGE(TAG, "Invalid URI: %s", this->uri.c_str());
        goto clean_up;
    }

    use_dtls = false;

    info_list = coap_resolve_address_info(&uri_t.host, uri_t.port, uri_t.port, 
                                          uri_t.port, uri_t.port, 0, 
                                          1 << uri_t.scheme, COAP_RESOLVE_TYPE_REMOTE);
    if (!info_list) {
        ESP_LOGE(TAG, "Address resolution failed for %.*s:%d", 
                 (int)uri_t.host.length, uri_t.host.s, uri_t.port);
        goto clean_up;
    }

    proto = info_list->proto;
    memcpy(&dst_addr, &info_list->addr, sizeof(dst_addr));
    coap_free_address_info(info_list);

    if (coap_uri_into_options(&uri_t, &dst_addr, &optlist, 1, uri_path, sizeof(uri_path)) < 0) {
        ESP_LOGE(TAG, "Failed to create URI options");
        goto clean_up;
    }

    ESP_LOGI(TAG, "Creating UDP CoAP session to %.*s:%d", 
             (int)uri_t.host.length, uri_t.host.s, uri_t.port);
    session = coap_new_client_session(ctx, nullptr, &dst_addr, proto);

    if (!session) {
        ESP_LOGE(TAG, "Failed to create CoAP session");
        goto clean_up;
    }

    request = coap_new_pdu(COAP_MESSAGE_CON, COAP_REQUEST_CODE_POST, session);
    if (!request) {
        ESP_LOGE(TAG, "Failed to create request PDU");
        goto clean_up;
    }

    coap_add_optlist_pdu(request, &optlist);

    // Add content format option
    {
        uint8_t format_buf[4];
        coap_add_option(request, COAP_OPTION_CONTENT_FORMAT,
                        coap_encode_var_safe(format_buf, sizeof(format_buf), 
                                            COAP_MEDIATYPE_APPLICATION_CBOR),
                        format_buf);
    }
    
    // Add the actual data
    if (!coap_add_data(request, bufferLength, buf_ptr)) {
        ESP_LOGE(TAG, "Failed to add data to PDU");
        goto clean_up;
    }

    ESP_LOGI(TAG, "Sending CoAP POST request...");
    resp_wait = true;
    mid = coap_send(session, request);
    if (mid == COAP_INVALID_MID) {
        ESP_LOGE(TAG, "coap_send() failed");
        resp_wait = false;
        goto clean_up;
    }

    // Wait for response with proper timeout
    while (resp_wait && elapsed_ms < total_wait_ms) {
        int result = coap_io_process(ctx, 100);
        if (result < 0) {
            ESP_LOGE(TAG, "coap_io_process() failed");
            break;
        }
        elapsed_ms += 100;
    }

    if (resp_wait) {
        ESP_LOGW(TAG, "No response from server within %d ms", total_wait_ms);
    }

clean_up:
    if (optlist) {
        coap_delete_optlist(optlist);
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