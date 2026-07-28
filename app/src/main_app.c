#include <stdio.h>
#include "log.h"
#include "stm32f1xx_hal.h"

#define APPLICATION_START_ADDR 0x08004000

void main_app(void)
{
    LOG_UART_Init();
    int i = 0;
    while (1)
    {
        // 读取PC地址判断出错位置
        uint32_t sp = __get_MSP();
        uint32_t pc;
        __asm volatile("mov %0, pc" : "=r"(pc));
        printf("This is a test message using printf. app start : %d, pc = %x, sp = %x\r\n", i, pc, sp);
        // HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
        // HAL_Delay(1000);
        // HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
        HAL_Delay(1000);
        for (volatile int j = 0; j < 1000*1000; j++);
        i++;
    }
}

void HardFault_Handler(void)
{
    // 1. 首先禁用中断（防止嵌套）
    __disable_irq();
    
    // 2. 读取故障信息到局部变量（不依赖外设）
    uint32_t sp = __get_MSP();
    uint32_t pc = *((uint32_t*)(sp + 24));
    uint32_t lr = *((uint32_t*)(sp + 20));
    uint32_t cfsr = SCB->CFSR;
    uint32_t hfsr = SCB->HFSR;
    printf("This is HardFault_Handler, pc = %x, sp = %x\r\n", pc, sp);
    // 3. 使用最简单的 GPIO 操作（不使用 HAL）
    // 直接操作寄存器，但要确保时钟已使能
    // 如果时钟未使能，使用硬件复位或简单循环
    while(1) {
        // 使用简单循环延迟（不依赖任何外设）
        for(volatile int i = 0; i < 1000000; i++);
    }
}