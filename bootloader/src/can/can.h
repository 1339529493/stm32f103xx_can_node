#ifndef _CAN_H_
#define _CAN_H_

#include "stm32f1xx_hal.h"

/* 私有宏定义 ----------------------------------------------------------------*/
#define CAN_TEST_MSG_ID      0x123   /* 测试消息 ID */
#define CAN_TEST_DATA_LEN    8       /* 测试数据长度 */
#define CAN_TEST_TIMEOUT     1000    /* 超时时间 (ms) */
#define CAN_RX_QUEUE_SIZE    32      /* 接收队列大小*/
#define CAN_RX_QUEUE_ID_SIZE       3    /* 接收队列ID个数*/
#define CAN_RX_FIFO          CAN_RX_FIFO0   /* 接收FIFO选择 */

/* 私有类型定义 --------------------------------------------------------------*/
typedef struct {
    uint32_t StdId;                 /* 标准ID */
    uint32_t ExtId;                 /* 扩展ID */
    uint32_t IDE;                   /* ID类型 */
    uint32_t RTR;                   /* 帧类型 */
    uint32_t DLC;                   /* 数据长度 */
    uint8_t Data[8];                /* 数据 */
    uint32_t Timestamp;             /* 时间戳 */
} CAN_RxMsg_t;

typedef struct {
    uint32_t can_id;
    volatile uint16_t g_rxHead;      /* 队列写指针 */
    volatile uint16_t g_rxTail;      /* 队列读指针 */
    volatile uint32_t g_droppedFrames;  /* 丢帧计数器 */
    volatile uint32_t g_receivedFrames;  /* 接收帧计数器 */
    void *state;
    CAN_RxMsg_t RxMsg[CAN_RX_QUEUE_SIZE];
} can_rx_queue_t;

void CAN1_Init(void);
void can_loop_test();
CAN_HandleTypeDef *CAN_GetHandle(void);
#endif