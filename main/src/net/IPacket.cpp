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
            ESP_LOGI(TAG, "CoAP response success (Code: %d.%02d)", rcvd_class, rcvd_code & 0x1F);
            
            // Log response payload if present
            size_t data_len;
            const uint8_t *data;
            if (coap_get_data(received, &data_len, &data) && data_len > 0) {
                ESP_LOGI(TAG, "Response payload: %zu bytes", data_len);
                uint16_t log_len = (data_len > 64) ? static_cast<uint16_t>(64) : static_cast<uint16_t>(data_len);
                ESP_LOG_BUFFER_HEX_LEVEL(TAG, data, log_len, ESP_LOG_INFO);
            }
            break;
            
        case CoAPResponseType::COAP_RESPONSE_CLIENT_ERROR:
            ESP_LOGE(TAG, "CoAP client error (%d.%02d)", rcvd_class, rcvd_code & 0x1F);
            break;
            
        case CoAPResponseType::COAP_RESPONSE_SERVER_ERROR:
            ESP_LOGE(TAG, "CoAP server error (%d.%02d)", rcvd_class, rcvd_code & 0x1F);
            break;
            
        default:
            ESP_LOGW(TAG, "CoAP unexpected response class: %d", rcvd_class);
            break;
    }

    resp_wait = false;
    return COAP_RESPONSE_OK;
}

void IPacket::sendPacket() {
    const uint8_t * buf_ptr = toBuffer();

    if (buf_ptr == nullptr || bufferLength == 0) {
        ESP_LOGE(TAG, "Packet buffer is null or empty — aborting send");
        return;
    }
    
    ESP_LOGI(TAG, "Preparing to send %zu bytes to URI: %s", bufferLength, this->uri.c_str());
    uint16_t log_len = (bufferLength > 64) ? static_cast<uint16_t>(64) : static_cast<uint16_t>(bufferLength);
    ESP_LOG_BUFFER_HEX_LEVEL(TAG, buf_ptr, log_len, ESP_LOG_INFO);

    coap_context_t * ctx = nullptr;
    coap_optlist_t * optlist = nullptr;
    coap_session_t * session = nullptr;
    coap_pdu_t * request = nullptr;
    coap_address_t dst_addr;
    coap_uri_t uri_t;
    coap_addr_info_t * info_list = nullptr;
    coap_proto_t proto;
    int total_wait_ms = 10000;  // 10 second timeout
    int elapsed_ms = 0;
    uint8_t uri_path[GEN_BUFFER_SIZE];
    coap_mid_t mid = COAP_INVALID_MID;

    coap_startup();

    // Create CoAP context
    ctx = coap_new_context(NULL);
    if (!ctx) {
        ESP_LOGE(TAG, "coap_new_context() failed");
        goto clean_up;
    }

    coap_context_set_block_mode(ctx, COAP_BLOCK_USE_LIBCOAP | COAP_BLOCK_SINGLE_BODY);
    coap_register_response_handler(ctx, IPacket::message_handler);

    // Parse URI
    if (coap_split_uri((const uint8_t *)this->uri.c_str(), strlen(this->uri.c_str()), &uri_t) == -1) {
        ESP_LOGE(TAG, "Invalid URI: %s", this->uri.c_str());
        goto clean_up;
    }

    ESP_LOGI(TAG, "Parsed URI - Host: %.*s, Port: %d, Scheme: %d", 
             static_cast<int>(uri_t.host.length), uri_t.host.s, uri_t.port, uri_t.scheme);

    // Resolve address - FIXED: Correct parameter order
    info_list = coap_resolve_address_info(
        &uri_t.host,
        uri_t.port,
        uri_t.port,
        uri_t.port,
        uri_t.port,
        AF_UNSPEC,              // Address family (AF_INET for IPv4, AF_INET6 for IPv6, AF_UNSPEC for both)
        1 << uri_t.scheme,      // Scheme hints
        COAP_RESOLVE_TYPE_REMOTE
    );
    
    if (!info_list) {
        ESP_LOGE(TAG, "Address resolution failed for %.*s:%d", 
                 static_cast<int>(uri_t.host.length), uri_t.host.s, uri_t.port);
        ESP_LOGE(TAG, "Make sure you are connected to WiFi and DNS is working");
        goto clean_up;
    }

    proto = info_list->proto;
    memcpy(&dst_addr, &info_list->addr, sizeof(dst_addr));
    
    // Log resolved address
    char addr_str[INET6_ADDRSTRLEN];
    if (dst_addr.addr.sa.sa_family == AF_INET) {
        inet_ntop(AF_INET, &dst_addr.addr.sin.sin_addr, addr_str, sizeof(addr_str));
        ESP_LOGI(TAG, "Resolved to IPv4: %s:%d", addr_str, ntohs(dst_addr.addr.sin.sin_port));
    } else if (dst_addr.addr.sa.sa_family == AF_INET6) {
        inet_ntop(AF_INET6, &dst_addr.addr.sin6.sin6_addr, addr_str, sizeof(addr_str));
        ESP_LOGI(TAG, "Resolved to IPv6: [%s]:%d", addr_str, ntohs(dst_addr.addr.sin6.sin6_port));
    }
    
    coap_free_address_info(info_list);
    info_list = nullptr;

    // Convert URI to CoAP options
    if (coap_uri_into_options(&uri_t, &dst_addr, &optlist, 1, uri_path, sizeof(uri_path)) < 0) {
        ESP_LOGE(TAG, "Failed to create URI options");
        goto clean_up;
    }

    // Create session
    ESP_LOGI(TAG, "Creating CoAP %s session", proto == COAP_PROTO_UDP ? "UDP" : "other");
    session = coap_new_client_session(ctx, nullptr, &dst_addr, proto);

    if (!session) {
        ESP_LOGE(TAG, "Failed to create CoAP session");
        goto clean_up;
    }

    // Create PDU
    request = coap_new_pdu(COAP_MESSAGE_CON, COAP_REQUEST_CODE_POST, session);
    if (!request) {
        ESP_LOGE(TAG, "Failed to create request PDU");
        goto clean_up;
    }

    // Add options
    if (!coap_add_optlist_pdu(request, &optlist)) {
        ESP_LOGE(TAG, "Failed to add options to PDU");
        goto clean_up;
    }

    // Add content format option (CBOR)
    {
        uint8_t format_buf[4];
        size_t format_len = coap_encode_var_safe(format_buf, sizeof(format_buf), 
                                                  COAP_MEDIATYPE_APPLICATION_CBOR);
        if (!coap_add_option(request, COAP_OPTION_CONTENT_FORMAT, format_len, format_buf)) {
            ESP_LOGE(TAG, "Failed to add content format option");
            goto clean_up;
        }
    }
    
    // Add the actual CBOR data
    if (!coap_add_data(request, bufferLength, buf_ptr)) {
        ESP_LOGE(TAG, "Failed to add data to PDU (size: %zu bytes)", bufferLength);
        goto clean_up;
    }

    ESP_LOGI(TAG, "Sending CoAP POST request with %zu bytes of data...", bufferLength);
    resp_wait = true;
    mid = coap_send(session, request);
    
    if (mid == COAP_INVALID_MID) {
        ESP_LOGE(TAG, "coap_send() failed - message ID invalid");
        resp_wait = false;
        goto clean_up;
    }
    
    ESP_LOGI(TAG, "CoAP message sent with MID: %d", mid);

    // Wait for response with proper timeout
    while (resp_wait && elapsed_ms < total_wait_ms) {
        int result = coap_io_process(ctx, 100);  // Process for 100ms
        if (result < 0) {
            ESP_LOGE(TAG, "coap_io_process() failed with code: %d", result);
            break;
        }
        elapsed_ms += 100;
        
        // Log progress every 2 seconds
        if (elapsed_ms % 2000 == 0) {
            ESP_LOGI(TAG, "Waiting for response... (%d ms elapsed)", elapsed_ms);
        }
    }

    if (resp_wait) {
        ESP_LOGW(TAG, "No response from server within %d ms - request may have succeeded but no ACK received", total_wait_ms);
    } else {
        ESP_LOGI(TAG, "Response received after %d ms", elapsed_ms);
    }

clean_up:
    if (optlist) {
        coap_delete_optlist(optlist);
        optlist = nullptr;
    }
    if (info_list) {
        coap_free_address_info(info_list);
        info_list = nullptr;
    }
    if (session) {
        coap_session_release(session);
        session = nullptr;
    }
    if (ctx) {
        coap_free_context(ctx);
        ctx = nullptr;
    }
    coap_cleanup();

    ESP_LOGI(TAG, "CoAP send finished");
}