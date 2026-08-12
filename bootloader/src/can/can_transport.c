#include "can_protocol.h"
#include "can.h"
#include <stdio.h>
#include <string.h>
/**
 * @brief 支持分片计算的 CRC16-CCITT
 * @param init_crc 初始 CRC 值。如果是第一片数据，传 0；如果是后续数据，传上一次计算的返回值。
 * @param data 数据指针
 * @param length 数据长度
 * @return 计算后的 CRC 值
 */
uint16_t crc16_ccitt_update(uint16_t init_crc, uint8_t *data, uint16_t length)
{
    uint8_t i;
    uint16_t crc = init_crc; // 使用传入的初始值，而不是固定为 0
    while(length--)
    {
        crc ^= *data++;
        for (i = 0; i < 8; ++i)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ 0x8408;
            else
                crc = (crc >> 1);
        }
    }
    return crc;
}

uint16_t crc16_ccitt(uint8_t *data, uint16_t length)
{
    return crc16_ccitt_update(0, data, length);
}

#define CAN_CRC_LEN 2
#define HEAD_FAREM_LEN 8
#define HEAD_FAREM_INIT(len, cmd, seq, total)   \
    {CAN_PRIVATE_SYNC0, CAN_PRIVATE_SYNC1, \
    ((len) & 0xff00) >> 8, (len) & 0xff, \
    ((cmd) & 0xff00) >> 8, (cmd) & 0xff, \
    (seq) & 0xff, (total) & 0xff}

/*
 * @brief CAN 数据发送 私有协议数据(head+data+crc) test canid 0x100
 * 
 * @param can_id CAN ID
 * @param cmd 命令
 * @param data 数据指针
 * @param len 数据长度
 * @return 0 成功，-1 失败
*/
int can_send_data_private(uint32_t can_id, uint16_t cmd,uint8_t *data, int len)
{
    return can_send_data_private_ex(can_id, cmd, data, len, 0, 1);
}

/*
 * @brief CAN 数据发送 私有协议数据(head+data+crc) test canid 0x100
 * 
 * @param can_id CAN ID
 * @param cmd 命令
 * @param data 数据指针
 * @param len 数据长度
 * @param seq 分片序号
 * @param frame_total 分片总数
 * @return 0 成功，-1 失败
*/
int can_send_data_private_ex(uint32_t can_id, uint16_t cmd, uint8_t *data, int len, uint8_t seq, uint8_t frame_total)
{
    uint8_t packet[HEAD_FAREM_LEN + MAX_PRIVATE_PAYLOAD_LEN + CAN_CRC_LEN];
    uint8_t tx_data[8];
    CAN_TxHeaderTypeDef tx_header;
    uint32_t tx_mailbox;
    uint16_t total_len;
    uint16_t crc;
    uint16_t offset;
    uint16_t chunk_len;
    uint32_t tick_start;
    HAL_StatusTypeDef status;

    if (len < 0 || len > MAX_PRIVATE_PAYLOAD_LEN) {
        return -1;
    }

    if ((len > 0) && (data == NULL)) {
        return -1;
    }

    if (frame_total == 0 || frame_total > CAN_PRIVATE_MAX_FRAME_COUNT) {
        return -1;
    }

    if (seq >= frame_total) {
        return -1;
    }

    total_len = HEAD_FAREM_LEN + CAN_CRC_LEN + (uint16_t)len;
    packet[0] = CAN_PRIVATE_SYNC0;
    packet[1] = CAN_PRIVATE_SYNC1;
    packet[2] = (uint8_t)((total_len >> 8) & 0xFF);
    packet[3] = (uint8_t)(total_len & 0xFF);
    packet[4] = (uint8_t)((cmd >> 8) & 0xFF);
    packet[5] = (uint8_t)(cmd & 0xFF);
    packet[6] = seq;
    packet[7] = frame_total;

    if (len > 0) {
        memcpy(&packet[HEAD_FAREM_LEN], data, len);
    }

    crc = crc16_ccitt(data, (uint16_t)len);
    packet[HEAD_FAREM_LEN + len] = (uint8_t)((crc >> 8) & 0xFF);
    packet[HEAD_FAREM_LEN + len + 1] = (uint8_t)(crc & 0xFF);

    tx_header.StdId = can_id;
    tx_header.ExtId = 0;
    tx_header.IDE = CAN_ID_STD;
    tx_header.RTR = CAN_RTR_DATA;
    tx_header.TransmitGlobalTime = DISABLE;

    offset = 0;
    while (offset < total_len) {
        chunk_len = (total_len - offset) > 8 ? 8 : (total_len - offset);
        memcpy(tx_data, &packet[offset], chunk_len);
        tx_header.DLC = chunk_len;

        status = HAL_CAN_AddTxMessage(CAN_GetHandle(), &tx_header, tx_data, &tx_mailbox);
        if (status != HAL_OK) {
            return -1;
        }

        tick_start = HAL_GetTick();
        while ((HAL_CAN_IsTxMessagePending(CAN_GetHandle(), tx_mailbox) != 0U) &&
               ((HAL_GetTick() - tick_start) < CAN_TEST_TIMEOUT)) {
        }

        if (HAL_GetTick() - tick_start >= CAN_TEST_TIMEOUT) {
            return -1;
        }

        offset += chunk_len;
    }

    return 0;
}

/**
 * @brief 重置单个 ID 的接收状态机
 */
static void reset_private_state(private_data_state_t *state)
{
    state->state = RX_STATE_IDLE;
    state->header_idx = 0;
    state->payload_idx = 0;
    state->total_len = 0;
    state->payload_len = 0;
    state->calc_crc = 0;
    state->recv_crc_high = 0;
}

/**
 * @brief CAN 单帧数据接收处理 (核心状态机)
 *
 * @param entry CAN ID 对应的缓冲区入口
 * @return int 
 *         1: 一帧完整数据接收并校验成功 (进入 FRAME_COMPLETE 状态)
 *         0: 接收中，或无数据，或正在处理头部/同步字
 *        -1: 校验失败、长度非法或协议错误，状态已重置
 */
static int can_rcve_data_private_process(can_rx_queue_t *entry)
{
    private_data_state_t *state = (private_data_state_t *)entry->state;
    if (state == NULL) return -1;

    // 如果已经处于完成状态，不再处理新数据，直到上层读取并重置
    if (state->state == RX_STATE_FRAME_COMPLETE) {
        return 1; 
    }

    uint8_t byte;
    CAN_RxMsg_t msg;
    if (CAN_QueuePop(entry->can_id, &msg) <= 0) {
        return 0; // 队列空，无数据
    }

    for (int i = 0; i < msg.DLC; i++)
    {
        byte = msg.Data[i];
        switch (state->state)
        {
        case RX_STATE_IDLE:
            if (byte == 0x55) {
                state->state = RX_STATE_SYNC_2;
            }
            break;

        case RX_STATE_SYNC_2:
            if (byte == 0xAA) {
                state->state = RX_STATE_HEADER;
                state->header_idx = 0;
                state->calc_crc = 0;
            } else {
                state->state = RX_STATE_IDLE; // 同步失败
            }
            break;

        case RX_STATE_HEADER:
            state->header_buf[state->header_idx] = byte;
            state->header_idx++;

            // 当收满 6 个字节后 (Len(2) + Cmd(2) + Res(2))
            if (state->header_idx >= 6) {
                uint16_t len = ((uint16_t)state->header_buf[0] << 8) | state->header_buf[1];

                state->frame_seq = state->header_buf[4];
                state->frame_total = state->header_buf[5];
                state->payload_len = len - HEAD_FAREM_LEN - CAN_CRC_LEN;

                // 合法性检查：防止内存越界
                if (len > MAX_PRIVATE_PAYLOAD_LEN) {
                    reset_private_state(state);
                    return -1;
                }

                state->payload_idx = 0;

                if (len == 0) {
                    // 如果没有 Payload，直接跳到 CRC 读取
                    state->state = RX_STATE_CRC_1;
                } else {
                    state->state = RX_STATE_PAYLOAD;
                }
            }
            break;

        case RX_STATE_PAYLOAD:
            // 保存数据到状态机缓冲区
            if (state->payload_idx < state->payload_len) {
                state->recv_buf[state->payload_idx] = byte;
                state->payload_idx++;

                // 如果 Payload 接收完毕
                if (state->payload_idx == state->payload_len) {
                    state->calc_crc = crc16_ccitt_update(state->calc_crc, state->recv_buf, state->payload_len);
                    state->state = RX_STATE_CRC_1;
                }
            } else {
                // 异常保护：索引超出范围，重置
                reset_private_state(state);
                return -1;
            }
            break;

        case RX_STATE_CRC_1:
            state->recv_crc_high = byte;
            state->state = RX_STATE_CRC_2;
            break;

        case RX_STATE_CRC_2:
            {
                uint16_t recv_crc = ((uint16_t)state->recv_crc_high << 8) | byte;
                if (recv_crc == state->calc_crc) {
                    // 校验成功，进入完成状态
                    state->state = RX_STATE_FRAME_COMPLETE; 
                    return 1; 
                } else {
                    reset_private_state(state);
                    return -1;
                }
            }
            break;

        default:
            reset_private_state(state);
            break;
        }
    }

    return 0;
}

/**
 * @brief CAN 数据接收 私有协议数据 (阻塞/非阻塞封装)
 * 
 * @param can_id CAN ID
 * @param cmd 命令
 * @param data 数据指针 (输出缓冲区)
 * @param len 数据长度指针 (输入:最大缓冲大小, 输出:实际接收到的有效数据长度)
 * @param timeout_ms 超时时间 (毫秒)。
 * @return 0 成功，其他 失败 (校验错误、超时、ID未找到、用户缓冲区太小)
*/
int can_rcve_data_private(uint32_t can_id,uint16_t cmd, uint8_t *data, int *len, int timeout_ms)
{
    if (data == NULL || len == NULL) return -1;
    can_rx_queue_t *entry = CAN_GetQueueById(can_id);
    if (entry == NULL  || entry->state == NULL) return -1; // ID 未找到

    private_data_state_t *state = (private_data_state_t *)entry->state;
    
    uint32_t start_tick = HAL_GetTick();
    uint32_t wait_tick = timeout_ms + start_tick;
    
    // 1. 循环调用 bit 接口，直到帧完成或出错
    while (1) {
        int ret = can_rcve_data_private_process(entry);
        
        if (ret == 1) {
            // 帧接收成功
            break;
        } else if (ret == -1) {
            // 校验错误或协议错误
            return -2;
        }
        
        // 检查超时
        if (timeout_ms > 0) {
            if (HAL_GetTick() - start_tick > wait_tick) {
                reset_private_state(state);
                return -3;
            }
        } else {

            return -4; 
        }
    }

    // 2. 帧已完成，从 state 中拷贝数据
    if (*len < state->payload_len) {
        reset_private_state(state);
        return -5; // 缓冲区太小
    }

    memcpy(data, state->recv_buf, state->payload_len);
    *len = state->payload_len;

    // 3. 重置状态机，准备下一帧
    reset_private_state(state);

    return 0;
}

void can_private_loop_test()
{
    uint8_t test_data[] = "Hello, this is a test message for CAN private protocol!";
    can_send_data_private(CAN_TEST_MSG_ID, 0x01, test_data, strlen((char *)test_data));
    uint8_t recv_buf[256];
    int recv_len = sizeof(recv_buf);
    int ret = can_rcve_data_private(CAN_TEST_MSG_ID, 0x01, recv_buf, &recv_len, 1000);
    if (ret == 0) {
        printf("Received: %s\r\n", recv_buf);
    } else {
        printf("Receive failed with error code: %d\r\n", ret);
    }
}