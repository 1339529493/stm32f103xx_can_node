#include <stdio.h>
#include "log.h"
#include "gpio.h"
#include "stm32f1xx_hal.h"

void main_app(void)
{
    LOG_UART_Init();
    MyPrintf("Hello, STM32!\n");
    printf("This is a test message using printf.\n");
    while (1)
    {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
        HAL_Delay(1000);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
        HAL_Delay(1000);
    }
}