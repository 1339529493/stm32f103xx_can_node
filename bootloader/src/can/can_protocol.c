#include "can_protocol.h"
#include "can.h"
#include <string.h>

// 在初始化时建立映射表（存放在RAM）
static int8_t can_id_map[2048];  // 0x001-0x7FF 范围

void CAN_ID_Map_Init(void)
{
    // 初始化所有为-1
    for (int i = 0; i < 2048; i++) can_id_map[i] = -1;
    // 注册需要的ID
    can_id_map[CAN_TEST_MSG_ID] = 0;
    can_id_map[0x002] = 1;
    can_id_map[0x003] = 2;
}

static inline int8_t CAN_ID_GetIndex(uint32_t stdId)
{
    if (stdId < 2048) {
        return can_id_map[stdId];
    }
    return -1;
}

private_data_state_t private_data_state[2];
can_rx_queue_t can_id_rvce_buffer[CAN_ID_MAX] = {
    {.can_id = CAN_TEST_MSG_ID, .state = &private_data_state[0]},
    {.can_id = 2, .state = &private_data_state[1]},
    {.can_id = 3, .state = NULL}
};

can_rx_queue_t *CAN_GetQueueById(uint32_t can_id)
{
    int8_t index = CAN_ID_GetIndex(can_id);
    if (index < 0) return NULL;
    return &can_id_rvce_buffer[index];
}

/* 队列操作函数 --------------------------------------------------------------*/
/**
  * @brief  向软件队列推送消息（中断/任务均可使用）
  * @param  pMsg: 消息指针
  * @retval 1: 成功, 0: 队列满
  */
uint8_t CAN_QueuePush(CAN_RxMsg_t *pMsg)
{
    int i = CAN_ID_GetIndex(pMsg->StdId);
    if (i < 0) return -1;
    uint16_t nextHead = (can_id_rvce_buffer[i].g_rxHead  + 1) % CAN_RX_QUEUE_SIZE;
    
    /* 检查队列是否已满 */
    if (nextHead == can_id_rvce_buffer[i].g_rxTail)
    {
        can_id_rvce_buffer[i].g_droppedFrames++;
        return 0;  /* 队列满 */
    }
    
    /* 复制数据到队列 */
    memcpy((void*)&(can_id_rvce_buffer[i].RxMsg[can_id_rvce_buffer[i].g_rxHead]), pMsg, sizeof(CAN_RxMsg_t));
    
    /* 更新写指针 */
    can_id_rvce_buffer[i].g_rxHead = nextHead;
    
    can_id_rvce_buffer[i].g_receivedFrames++;
    return 1;  /* 成功 */
}

/**
  * @brief  从软件队列弹出消息（任务上下文使用）
  * @param  pMsg: 消息指针（用于输出）
  * @retval 1: 成功, 0: 队列空
  */
uint8_t CAN_QueuePop(uint32_t can_id ,CAN_RxMsg_t *pMsg)
{
    int i = CAN_ID_GetIndex(can_id);
    if (i < 0) return -1;

    if (can_id_rvce_buffer[i].g_rxHead == can_id_rvce_buffer[i].g_rxTail)
    {
        return 0;  /* 队列空 */
    }
    
    /* 复制数据 */
    memcpy(pMsg, (void*)&can_id_rvce_buffer[i].RxMsg[can_id_rvce_buffer[i].g_rxTail], sizeof(CAN_RxMsg_t));
    
    /* 更新读指针 */
    can_id_rvce_buffer[i].g_rxTail = (can_id_rvce_buffer[i].g_rxTail + 1) % CAN_RX_QUEUE_SIZE;
    
    return 1;
}

/**
  * @brief  获取队列当前使用量
  * @retval 队列中的消息数量
  */
uint16_t CAN_QueueGetCount(uint32_t can_id)
{
    int i = CAN_ID_GetIndex(can_id);
    if (i < 0) return -1;
    if (can_id_rvce_buffer[i].g_rxHead >= can_id_rvce_buffer[i].g_rxTail)
    {
        return (can_id_rvce_buffer[i].g_rxHead - can_id_rvce_buffer[i].g_rxTail);
    }
    else
    {
        return (CAN_RX_QUEUE_SIZE - can_id_rvce_buffer[i].g_rxTail + can_id_rvce_buffer[i].g_rxHead);
    }
}
