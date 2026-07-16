#include <stdio.h>
#include <stdlib.h>
#include <string.h>   // 添加：用于 memset, strlen
#include <stdarg.h>   // 添加：用于 va_list, va_start, va_end
#include "stm32f1xx_hal.h"

#define TX_BUF_LEN 256     /* 发送缓冲区容量，根据需要进行调整 */
uint8_t TxBuf[TX_BUF_LEN]; /* 发送缓冲区                       */
UART_HandleTypeDef huart1; /* 串口句柄，根据实际情况进行调整 */

void LOG_UART_Init()
{
    __HAL_RCC_USART1_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    /* 配置 TX (PA9) 为复用推挽输出 */
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;      // 复用推挽
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    /* 配置 RX (PA10) 为复用浮空输入 */
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    /* 串口初始化 */
    huart1.Instance = USART1;                         // 选择USART1外设
    huart1.Init.BaudRate = 115200;                    // 波特率
    huart1.Init.WordLength = UART_WORDLENGTH_8B;      // 数据位：8位
    huart1.Init.StopBits = UART_STOPBITS_1;           // 停止位：1位
    huart1.Init.Parity = UART_PARITY_NONE;            // 校验位：无
    huart1.Init.Mode = UART_MODE_TX_RX;               // 模式：同时使能发送和接收
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;      // 硬件流控：无
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;  // 过采样：16
    if (HAL_UART_Init(&huart1) != HAL_OK) {
        Error_Handler();
    
}

void MyPrintf(const char *__format, ...)
{
    va_list ap;
    va_start(ap, __format);

    /* 清空发送缓冲区 */
    memset(TxBuf, 0x0, TX_BUF_LEN);

    /* 填充发送缓冲区 */
    vsnprintf((char *)TxBuf, TX_BUF_LEN, (const char *)__format, ap);
    va_end(ap);
    int len = strlen((const char *)TxBuf);

    /* 往串口发送数据 */
    HAL_UART_Transmit(&huart1, (uint8_t *)&TxBuf, len, 0xFFFF);
}

int _write(int fd, char *ptr, int len)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, 0xFFFF);
    return len;
}