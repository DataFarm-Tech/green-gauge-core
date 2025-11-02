#include <esp_log.h>
#include <cstring>

#include <cbor.h>
#include <sys/socket.h>
#include "esp_log.h"
#include "coap3/coap.h"
#include <cstdint>
#include <cstddef>
#include "packet/IPacket.hpp"
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
    const uint8_t * buffer = toBuffer();
    coap_context_t * ctx = nullptr;
    coap_optlist_t * optlist = nullptr;
    coap_session_t * session = nullptr;
    coap_pdu_t * request = nullptr;
    coap_address_t dst_addr;
    static coap_uri_t uri;
    coap_addr_info_t * info_list = nullptr;
    coap_proto_t proto;
    uint16_t wait_ms = 500;

    uint8_t uri_path[BUFFER_SIZE];

    coap_startup();

    ctx = coap_new_context(NULL);
    if (!ctx) {
        ESP_LOGE(TAG, "coap_new_context() failed");
        goto clean_up;
    }

    coap_context_set_block_mode(ctx, COAP_BLOCK_USE_LIBCOAP | COAP_BLOCK_SINGLE_BODY);
    coap_register_response_handler(ctx, IPacket::message_handler);

    if (coap_split_uri((const uint8_t *)this->uri.c_str(), strlen(this->uri.c_str()), &uri) == -1) {
        ESP_LOGE(TAG, "Invalid URI: %s", this->uri.c_str());
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
    coap_add_option(request, COAP_OPTION_CONTENT_FORMAT,
                    coap_encode_var_safe(buf, sizeof(buf), COAP_MEDIATYPE_APPLICATION_CBOR),
                    buf);
    coap_add_data(request, bufferLength, buffer);

    resp_wait = true;
    coap_send(session, request);

    while (resp_wait) {
        int result = coap_io_process(ctx, wait_ms > 100 ? 100 : wait_ms);
        if (result >= 0) {
            if (result >= wait_ms) {
                ESP_LOGE(TAG, "No response from server within 500 ms");
                break;
            } else {
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