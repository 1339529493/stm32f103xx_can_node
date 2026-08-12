#ifndef __CAN_PROTOCOL_H
#define __CAN_PROTOCOL_H

#include "can.h"

#define CAN_ID_MAX 3
#define CAN_PRIVATE_SYNC0 0x55
#define CAN_PRIVATE_SYNC1 0xAA
#define CAN_PRIVATE_HEADER_LEN 8
#define CAN_PRIVATE_MAX_FRAME_COUNT 255

typedef enum {
    CAN_CMD_DATA = 0x0000,
    CAN_CMD_FW_START = 0x1000,
    CAN_CMD_FW_DATA = 0x1001,
    CAN_CMD_FW_ACK = 0x1002,
    CAN_CMD_FW_NACK = 0x1003,
    CAN_CMD_FW_VERIFY = 0x1004,
    CAN_CMD_FW_JUMP = 0x1005,
} can_private_cmd_e;

typedef enum {
    RX_STATE_IDLE = 0,
    RX_STATE_SYNC_1,      // 等待 0x55 
    RX_STATE_SYNC_2,      // 等待 0xAA
    RX_STATE_HEADER,      // 读取剩余 6 字节头
    RX_STATE_PAYLOAD,     // 读取 Payload
    RX_STATE_CRC_1,       // 读取 CRC 高字节
    RX_STATE_CRC_2,       // 读取 CRC 低字节
    RX_STATE_FRAME_COMPLETE // 新增：一帧接收完成，等待上层读取
} private_data_state_e;

#define MAX_PRIVATE_PAYLOAD_LEN 256 // 根据实际需求调整最大负载长度

typedef struct {
    private_data_state_e state;
    uint8_t header_buf[6];
    uint8_t header_idx;
    uint16_t total_len;
    uint16_t payload_len;
    uint16_t payload_idx;
    uint16_t calc_crc;
    uint8_t recv_crc_high;
    uint8_t frame_seq;
    uint8_t frame_total;
    
    // 新增：接收数据缓冲区
    uint8_t recv_buf[MAX_PRIVATE_PAYLOAD_LEN]; 
} private_data_state_t;

int can_send_data_private(uint32_t can_id, uint16_t cmd,uint8_t *data, int len);
int can_send_data_private_ex(uint32_t can_id, uint16_t cmd, uint8_t *data, int len, uint8_t seq, uint8_t frame_total);
int can_rcve_data_private(uint32_t can_id,uint16_t cmd, uint8_t *data, int *len, int timeout_ms);
void can_private_loop_test();

void CAN_ID_Map_Init(void);
uint8_t CAN_QueuePush(CAN_RxMsg_t *pMsg);
uint8_t CAN_QueuePop(uint32_t can_id, CAN_RxMsg_t *pMsg);
uint16_t CAN_QueueGetCount(uint32_t can_id);
can_rx_queue_t *CAN_GetQueueById(uint32_t can_id);
#endif
