#include <stdio.h>
#include "log.h"
#include "stm32f1xx_hal.h"
#include "can.h"

#define APPLICATION_START_ADDR 0x08004000

/**
 * @brief 禁用所有中断
 */
void bootloader_disable_interrupts(void)
{
    __disable_irq();
    __set_PRIMASK(0);
    // 清理挂起的中断（防止跳转后立即触发）
    // for (int i = 0; i < 32; i++) {
    //     NVIC_ClearPendingIRQ((IRQn_Type)i);
    // }
}

/**
 * @brief 去初始化外设
 */
void bootloader_deinit_peripherals(void)
{
    LOG_UART_DeInit();
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_13);
    // 2. 复位所有外设时钟（关键！）
    HAL_RCC_DeInit();
    HAL_DeInit();
    // 3. 关闭SysTick，清空计数器
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;
    // 数据同步屏障（确保所有操作完成）
    __DSB();
    __ISB();
}

/**
 * @brief 将向量表设置为应用程序地址
 */
static void bootloader_set_application_vector_table(void)
{
    SCB->VTOR = APPLICATION_START_ADDR;
}

/**
 * @brief 跳转到应用程序
 */
__attribute__((noreturn)) void bootloader_jump_to_application(void)
{
    uint32_t app_stack_ptr = *((uint32_t *)APPLICATION_START_ADDR);
    uint32_t app_reset_vector = *((uint32_t *)(APPLICATION_START_ADDR + 4));

    // ===== 日志1：打印向量表内容 =====
    printf("========== Bootloader Jump Info ==========\r\n");
    printf("APP_START_ADDR: 0x%08X\r\n", APPLICATION_START_ADDR);
    printf("MSP (from flash): 0x%08lX\r\n", app_stack_ptr);
    printf("PC (from flash): 0x%08lX\r\n", app_reset_vector);
    
    // ===== 日志2：验证向量表 =====
    if (app_stack_ptr == 0xFFFFFFFF || app_stack_ptr == 0x00000000) {
        printf("ERROR: Flash is EMPTY! (0xFFFFFFFF or 0x00000000)\r\n");
        printf("Please burn APP firmware first!\r\n");
        return;
    }
    
    if (app_stack_ptr < 0x20000000 || app_stack_ptr > 0x20005000) {
        printf("ERROR: Invalid MSP! RAM range: 0x20000000 ~ 0x20005000\r\n");
        return;
    }
    
    if ((app_reset_vector & 0x01) == 0) {
        printf("ERROR: PC is not Thumb mode (LSB=0)!\r\n");
        return;
    }
    
    // ===== 日志3：打印前32字节内容 =====
    printf("\r\nFlash content at 0x08004000 (32 bytes):\r\n");
    for (int i = 0; i < 32; i++) {
        printf("%02X ", *((uint8_t*)(APPLICATION_START_ADDR + i)));
        if ((i + 1) % 16 == 0) printf("\r\n");
    }
    
    // ===== 日志4：打印当前状态 =====
    printf("\r\nCurrent CPU state:\r\n");
    printf("MSP: 0x%08lX\r\n", __get_MSP());
    printf("PSP: 0x%08lX\r\n", __get_PSP());
    printf("CONTROL: 0x%08lX\r\n", __get_CONTROL());
    printf("PRIMASK: 0x%08lX\r\n", __get_PRIMASK());
    printf("VTOR: 0x%08lX\r\n", SCB->VTOR);
    
    // ===== 日志5：准备跳转 =====
    printf("\r\nJumping to APP...\r\n");
    printf("==========================================\r\n");

    /* 应用程序复位处理程序的函数指针 */
    void (*app_reset_handler)(void) = (void (*)(void))(app_reset_vector);

    /* 禁用所有中断 */
    bootloader_disable_interrupts();
    printf("Jumping to application...\r\n");

    /* 去初始化外设 */
    bootloader_deinit_peripherals();

    /* 将向量表设置为应用程序地址 */
    bootloader_set_application_vector_table();

    /* 设置主堆栈指针 */
    __set_MSP(app_stack_ptr);
    // __set_CONTROL(0);
    // __DSB();
    // __ISB();

    /* 跳转到应用程序 */
    app_reset_handler();

    while (1)
    {

    }
}

void main_bootloader(void)
{
    LOG_UART_Init();
    CAN1_Init();
    int i = 0;
    while (1)
    {
        printf("This is a test message using printf. bootloader start : %d\r\n", i);
        // HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
        // HAL_Delay(1000);
        // HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
        HAL_Delay(1000);
        i++;
        if (i > 2)
        {
            // bootloader_jump_to_application();
        }
        can_loop_test();
    }
}

void HardFault_Handler(void)
{

}