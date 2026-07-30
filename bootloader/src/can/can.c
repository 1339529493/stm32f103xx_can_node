/**
  ******************************************************************************
  * @file    can_loopback_test.c
  * @brief   CAN 回环测试程序（中断接收版本）
  * @note    使用环回模式，中断方式接收，软件队列缓存
  ******************************************************************************
  */
#include <stdio.h>
#include <string.h>
#include "can.h"
#include "main.h"

/* 私有变量 ------------------------------------------------------------------*/
static CAN_HandleTypeDef hcan1;                 /* CAN 句柄 */
static CAN_RxMsg_t g_canRxQueue[CAN_RX_QUEUE_SIZE]; /* 软件接收队列 */
static volatile uint16_t g_rxHead = 0;          /* 队列写指针 */
static volatile uint16_t g_rxTail = 0;          /* 队列读指针 */
static volatile uint32_t g_droppedFrames = 0;   /* 丢帧计数器 */
static volatile uint32_t g_receivedFrames = 0;  /* 接收帧计数器 */

/* 函数声明 ------------------------------------------------------------------*/
static void CAN_GPIO_Init(void);
static void CAN_NVIC_Init(void);
static uint8_t CAN_QueuePush(CAN_RxMsg_t *pMsg);
static uint8_t CAN_QueuePop(CAN_RxMsg_t *pMsg);
static HAL_StatusTypeDef CAN_SendTestMessage(void);
static HAL_StatusTypeDef CAN_ReceiveTestMessage(void);

/* 中断回调函数 --------------------------------------------------------------*/
/**
  * @brief  CAN FIFO0 消息等待回调函数（中断上下文）
  * @param  hcan: CAN 句柄
  * @retval 无
  */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxMsg_t msg;
    CAN_RxHeaderTypeDef rxHeader;
    uint8_t rxData[8];
    
    /* ⚡ 关键：中断中立即读取FIFO，释放硬件资源 */
    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO, &rxHeader, rxData) == HAL_OK)
    {
        /* 组装消息到软件队列 */
        msg.StdId = rxHeader.StdId;
        msg.ExtId = rxHeader.ExtId;
        msg.IDE = rxHeader.IDE;
        msg.RTR = rxHeader.RTR;
        msg.DLC = rxHeader.DLC;
        msg.Timestamp = rxHeader.Timestamp;
        memcpy(msg.Data, rxData, 8);
        
        /* 放入软件队列（中断安全版本） */
        if (CAN_QueuePush(&msg) == 1)
        {
            g_receivedFrames++;
        }
        else
        {
            g_droppedFrames++;  /* 队列满，丢帧 */
        }
    }
}

/**
  * @brief  CAN 错误回调函数
  * @param  hcan: CAN 句柄
  * @retval 无
  */
void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan)
{
    uint32_t errorCode = HAL_CAN_GetError(hcan);
    
    /* 处理各种错误 */
    if (errorCode & HAL_CAN_ERROR_RX_FOV0)
    {
        /* FIFO0 溢出错误（中断读取太慢才会发生） */
        /* 实际上，如果中断及时读取，不应该发生 */
    }
    
    if (errorCode & HAL_CAN_ERROR_BOF)
    {
        /* 总线关闭错误 */
    }
    
    /* 其他错误处理... */
}

/**
  * @brief  CAN 发送完成回调（可选）
  * @param  hcan: CAN 句柄
  * @retval 无
  */
void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef *hcan)
{
    /* 发送完成处理（可添加信号量等） */
    printf("CAN send sucusse\r\n");
}

/**
  * @brief  CAN 发送完成回调（可选）
  * @param  hcan: CAN 句柄
  * @retval 无
  */
void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef *hcan)
{
    /* 发送完成处理 */
    printf("CAN send sucusse\r\n");
}

/**
  * @brief  CAN 发送完成回调（可选）
  * @param  hcan: CAN 句柄
  * @retval 无
  */
void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef *hcan)
{
    /* 发送完成处理 */
    printf("CAN send sucusse\r\n");
}

/* 队列操作函数 --------------------------------------------------------------*/
/**
  * @brief  向软件队列推送消息（中断/任务均可使用）
  * @param  pMsg: 消息指针
  * @retval 1: 成功, 0: 队列满
  */
static uint8_t CAN_QueuePush(CAN_RxMsg_t *pMsg)
{
    uint16_t nextHead = (g_rxHead + 1) % CAN_RX_QUEUE_SIZE;
    
    /* 检查队列是否已满 */
    if (nextHead == g_rxTail)
    {
        return 0;  /* 队列满 */
    }
    
    /* 复制数据到队列 */
    memcpy((void*)&g_canRxQueue[g_rxHead], pMsg, sizeof(CAN_RxMsg_t));
    
    /* 更新写指针 */
    g_rxHead = nextHead;
    
    return 1;  /* 成功 */
}

/**
  * @brief  从软件队列弹出消息（任务上下文使用）
  * @param  pMsg: 消息指针（用于输出）
  * @retval 1: 成功, 0: 队列空
  */
static uint8_t CAN_QueuePop(CAN_RxMsg_t *pMsg)
{
    /* 检查队列是否为空 */
    if (g_rxHead == g_rxTail)
    {
        return 0;  /* 队列空 */
    }
    
    /* 复制数据 */
    memcpy(pMsg, (void*)&g_canRxQueue[g_rxTail], sizeof(CAN_RxMsg_t));
    
    /* 更新读指针 */
    g_rxTail = (g_rxTail + 1) % CAN_RX_QUEUE_SIZE;
    
    return 1;  /* 成功 */
}

/**
  * @brief  获取队列当前使用量
  * @retval 队列中的消息数量
  */
static uint16_t CAN_QueueGetCount(void)
{
    if (g_rxHead >= g_rxTail)
    {
        return (g_rxHead - g_rxTail);
    }
    else
    {
        return (CAN_RX_QUEUE_SIZE - g_rxTail + g_rxHead);
    }
}

/* 初始化函数 ----------------------------------------------------------------*/
/**
  * @brief  CAN 初始化配置（含中断使能）
  * @param  无
  * @retval 无
  */
void CAN1_Init(void)
{
    CAN_GPIO_Init();
    CAN_FilterTypeDef sFilterConfig;
    
    /* CAN 基本参数配置 */
    hcan1.Instance = CAN1;
    hcan1.Init.Prescaler = 36;              /* 预分频器: 36 */
    hcan1.Init.Mode = CAN_MODE_LOOPBACK;    /* 环回模式，无需外部节点 */
    hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ; /* 同步跳转宽度: 1 TQ */
    hcan1.Init.TimeSeg1 = CAN_BS1_4TQ;      /* 位段1: 4 TQ */
    hcan1.Init.TimeSeg2 = CAN_BS2_3TQ;      /* 位段2: 3 TQ */
    hcan1.Init.TimeTriggeredMode = DISABLE; /* 禁用时间触发模式 */
    hcan1.Init.AutoBusOff = DISABLE;        /* 禁用自动总线关闭 */
    hcan1.Init.AutoWakeUp = DISABLE;        /* 禁用自动唤醒 */
    hcan1.Init.AutoRetransmission = ENABLE; /* 启用自动重传 */
    hcan1.Init.ReceiveFifoLocked = DISABLE; /* 禁用接收FIFO锁定 */
    hcan1.Init.TransmitFifoPriority = DISABLE; /* 禁用发送FIFO优先级 */
    
    if (HAL_CAN_Init(&hcan1) != HAL_OK)
    {
        Error_Handler();
    }
    
    /* 配置过滤器 - 接收所有消息 */
    sFilterConfig.FilterBank = 0;
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
    sFilterConfig.FilterIdHigh = 0x0000;
    sFilterConfig.FilterIdLow = 0x0000;
    sFilterConfig.FilterMaskIdHigh = 0x0000;
    sFilterConfig.FilterMaskIdLow = 0x0000;
    sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO;
    sFilterConfig.FilterActivation = CAN_FILTER_ENABLE;
    sFilterConfig.SlaveStartFilterBank = 14;
    
    if (HAL_CAN_ConfigFilter(&hcan1, &sFilterConfig) != HAL_OK)
    {
        Error_Handler();
    }
    CAN_NVIC_Init();
    HAL_CAN_Start(&hcan1);
    HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);
}

/**
  * @brief  NVIC 初始化（使能CAN中断）
  * @param  无
  * @retval 无
  */
static void CAN_NVIC_Init(void)
{
    /* 设置CAN中断优先级 */
    HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);
    
    /* 如果有错误中断需求 */
    HAL_NVIC_SetPriority(CAN1_SCE_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(CAN1_SCE_IRQn);
    
    /* 发送中断（根据需要） */
    HAL_NVIC_SetPriority(CAN1_TX_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(CAN1_TX_IRQn);
}

/**
  * @brief  GPIO 初始化
  * @param  无
  * @retval 无
  */
static void CAN_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    /* 使能时钟 */
    __HAL_RCC_CAN1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    /* CAN TX (PA12) - 复用推挽输出 */
    GPIO_InitStruct.Pin = GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    /* CAN RX (PA11) - 复用浮空输入 */
    GPIO_InitStruct.Pin = GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/* 中断服务函数 --------------------------------------------------------------*/
/**
  * @brief  CAN1 接收中断服务函数
  * @param  无
  * @retval 无
  */
void CAN1_RX0_IRQHandler(void)
{
    HAL_CAN_IRQHandler(&hcan1);
}

/**
  * @brief  CAN1 发送中断服务函数
  * @param  无
  * @retval 无
  */
void CAN1_TX_IRQHandler(void)
{
    HAL_CAN_IRQHandler(&hcan1);
}

/**
  * @brief  CAN1 状态/错误中断服务函数
  * @param  无
  * @retval 无
  */
void CAN1_SCE_IRQHandler(void)
{
    HAL_CAN_IRQHandler(&hcan1);
}












/* CAN操作函数 ----------------------------------------------------------------*/
/**
  * @brief  发送测试消息
  * @param  无
  * @retval HAL_OK: 发送成功, 其他: 发送失败
  */
HAL_StatusTypeDef CAN_SendTestMessage(void)
{
    CAN_TxHeaderTypeDef txHeader;
    uint8_t txData[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    uint32_t txMailbox;
    HAL_StatusTypeDef status;
    uint32_t tickstart;
    
    /* 配置发送消息头 */
    txHeader.StdId = CAN_TEST_MSG_ID;
    txHeader.ExtId = 0;
    txHeader.IDE = CAN_ID_STD;          /* 标准ID */
    txHeader.RTR = CAN_RTR_DATA;        /* 数据帧 */
    txHeader.DLC = CAN_TEST_DATA_LEN;
    txHeader.TransmitGlobalTime = DISABLE;
    
    /* 发送消息（非阻塞，立即返回） */
    status = HAL_CAN_AddTxMessage(&hcan1, &txHeader, txData, &txMailbox);
    
    if (status == HAL_OK)
    {
        /* 等待发送完成（也可使用中断回调，这里使用轮询） */
        tickstart = HAL_GetTick();
        while ((HAL_CAN_IsTxMessagePending(&hcan1, txMailbox) != 0U) &&
               ((HAL_GetTick() - tickstart) < CAN_TEST_TIMEOUT))
        {
            /* 等待发送完成 */
        }
        
        if (HAL_GetTick() - tickstart >= CAN_TEST_TIMEOUT)
        {
            printf("CAN_SendTestMessage() timeout ,error : %x\r\n",hcan1.ErrorCode);
            return HAL_TIMEOUT;
        }
    }
    else
    {
        printf("CAN_SendTestMessage() failed ,error : %x\r\n",hcan1.ErrorCode);
    }
    
    return status;
}

/**
  * @brief  从软件队列接收消息
  * @param  无
  * @retval HAL_OK: 接收成功, HAL_TIMEOUT: 超时, HAL_ERROR: 数据错误
  */
HAL_StatusTypeDef CAN_ReceiveTestMessage(void)
{
    CAN_RxMsg_t msg;
    uint32_t tickstart = HAL_GetTick();
    
    /* 等待队列中有数据（超时保护） */
    while (CAN_QueueGetCount() == 0)
    {
        if ((HAL_GetTick() - tickstart) >= CAN_TEST_TIMEOUT)
        {
            return HAL_TIMEOUT;
        }
        /* 建议添加一个小延时，避免空转 */
        HAL_Delay(1);
    }
    
    /* 从软件队列弹出消息 */
    if (CAN_QueuePop(&msg) == 0)
    {
        return HAL_ERROR;
    }
    
    /* 验证消息ID */
    if (msg.StdId != CAN_TEST_MSG_ID)
    {
        return HAL_ERROR;
    }
    
    if (msg.DLC != CAN_TEST_DATA_LEN)
    {
        return HAL_ERROR;
    }
    
    printf("Received message: %02X %02X %02X %02X %02X %02X %02X %02X\r\n",
           msg.Data[0], msg.Data[1], msg.Data[2], msg.Data[3],
           msg.Data[4], msg.Data[5], msg.Data[6], msg.Data[7]);

    
    return HAL_OK;
}

void can_loop_test()
{
    HAL_StatusTypeDef ret;
    ret = CAN_SendTestMessage();
    if (ret != HAL_OK)
    {
        printf("CAN_SendTestMessage() failed ,error : %x\r\n",ret);
    }
    ret = CAN_ReceiveTestMessage();
    if (ret != HAL_OK)
    {
        printf("CAN_ReceiveTestMessage() failed ,error : %x\r\n",ret);
    }
}