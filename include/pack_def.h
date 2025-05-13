#pragma once
#include <Arduino.h>
#include "crypt.h"
#include "msg_queue.h"

#define ADDRESS_SIZE 6
#define DATA_SIZE 7
#define PACKET_LENGTH 55

#define CN001_REQ_LEN 49
#define SN001_SUC_RSP_LEN 55
#define SN001_ERR_RSP_LEN 49

#define CN001_REQ_ID 0x01
#define SN001_SUC_RSP_ID 0x02
#define SN001_ERR_RSP_ID 0x03

/**
 * The following defines are user to identify the
 * different types of error responses that can be sent.
 * 
 * The error codes are as follows:
 * SN001_ERR_RSP_CODE_A 0x0A - No connection to RS485
 * SN001_ERR_RSP_CODE_B 0x0B - Invalid data from RS485 (upper bounds and lower bounds)
 */
#define SN001_ERR_RSP_CODE_A 0x0A
#define SN001_ERR_RSP_CODE_B 0x0B
#define SN001_SUC_RSP_CODE 0x0C


/** 
 * @brief The following structure represents a
 * packet for the CN001 request. This packet is used to
 * request data from a node in the network. When creating this structure,
 * the msg_id field should be set to 0x01. 
 */
typedef struct {
    char des_node[ADDRESS_SIZE + 1];
    char src_node[ADDRESS_SIZE + 1];
    uint8_t num_nodes;
    uint8_t ttl;
} cn001_req;

/**
 * @brief The following structure represents a
 * packet for the SN001 response. This packet is used to
 * respond to a CN001 request. When creating this structure,
 * the msg_id field should be set to 0x02.
 */
typedef struct {
    char des_node[ADDRESS_SIZE + 1];
    char src_node[ADDRESS_SIZE + 1];
    char data[DATA_SIZE];
    uint8_t ttl;
} sn001_rsp;

/**
 * @brief The following structure represents a
 * packet for the SN001 error response. This packet is used to
 * respond to a CN001 request with an error. When creating this structure,
 * the msg_id field should be set to 0x03, 0x04, etc. See ERROR codes.
 */
typedef struct {
    char des_node[ADDRESS_SIZE + 1];
    char src_node[ADDRESS_SIZE + 1];
    uint8_t ttl;
    uint8_t hash[SHA256_SIZE];
    uint8_t err_code;
} sn001_err_rsp;

void pkt_cn001_req(uint8_t * buf, const cn001_req * pkt, uint8_t seq_id);
void pkt_sn001_rsp(uint8_t * buf, const sn001_rsp * pkt, uint8_t seq_id);
void pkt_sn001_err_rsp(uint8_t * buf, const sn001_err_rsp * pkt, uint8_t seq_id);

void cn001_handle_packet(const uint8_t *buf, uint8_t buf_len);
