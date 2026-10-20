/**
 *
 * @file        startup_stm32f405.c
 * @brief       STM32F405xx Devices vector table for ATfE based toolchains.
 * @author      Aleksander Dziągwa
 * 
 */

#include <stdint.h>
#include "stm32f405xx.h"

#define WEAK __attribute__((weak, alias("Default_Handler"), noreturn))

/* Linker script symbols for RAM */
extern uint32_t __stack;
extern uint32_t __data_source;
extern uint32_t __data_start;
extern uint32_t __data_size;
extern uint32_t __data_end;

extern uint32_t __bss_start;
extern uint32_t __bss_end;

/* Linker script symbols for CCMRAM */
extern uint32_t __ccmram_source;
extern uint32_t __ccmram_start;
extern uint32_t __ccmram_end;
extern uint32_t __ccmram_bss_start;
extern uint32_t __ccmram_bss_end;

/* Linker script symbols for TLS */
extern uint32_t __tdata_source;
extern uint32_t __tls_base;
extern uint32_t __tdata_end;
extern uint32_t __tls_end;

/* Picolibc function to set the TLS pointer for the current thread */
extern void _set_tls(void *tls);

extern void SystemInit(void);
extern void __libc_init_array(void);
extern int main(void);

void Reset_Handler(void) __attribute__((noreturn));
void Default_Handler(void) __attribute__((noreturn));

/* Core Exceptions */
void WEAK NMI_Handler(void);
void WEAK HardFault_Handler(void);
void WEAK MemManage_Handler(void);
void WEAK BusFault_Handler(void);
void WEAK UsageFault_Handler(void);
void WEAK SVC_Handler(void);
void WEAK DebugMon_Handler(void);
void WEAK PendSV_Handler(void);
void WEAK SysTick_Handler(void);

/* STM32F405 Interrupts */
void WEAK WWDG_IRQHandler(void);
void WEAK PVD_IRQHandler(void);
void WEAK TAMP_STAMP_IRQHandler(void);
void WEAK RTC_WKUP_IRQHandler(void);
void WEAK FLASH_IRQHandler(void);
void WEAK RCC_IRQHandler(void);
void WEAK EXTI0_IRQHandler(void);
void WEAK EXTI1_IRQHandler(void);
void WEAK EXTI2_IRQHandler(void);
void WEAK EXTI3_IRQHandler(void);
void WEAK EXTI4_IRQHandler(void);
void WEAK DMA1_Stream0_IRQHandler(void);
void WEAK DMA1_Stream1_IRQHandler(void);
void WEAK DMA1_Stream2_IRQHandler(void);
void WEAK DMA1_Stream3_IRQHandler(void);
void WEAK DMA1_Stream4_IRQHandler(void);
void WEAK DMA1_Stream5_IRQHandler(void);
void WEAK DMA1_Stream6_IRQHandler(void);
void WEAK ADC_IRQHandler(void);
void WEAK CAN1_TX_IRQHandler(void);
void WEAK CAN1_RX0_IRQHandler(void);
void WEAK CAN1_RX1_IRQHandler(void);
void WEAK CAN1_SCE_IRQHandler(void);
void WEAK EXTI9_5_IRQHandler(void);
void WEAK TIM1_BRK_TIM9_IRQHandler(void);
void WEAK TIM1_UP_TIM10_IRQHandler(void);
void WEAK TIM1_TRG_COM_TIM11_IRQHandler(void);
void WEAK TIM1_CC_IRQHandler(void);
void WEAK TIM2_IRQHandler(void);
void WEAK TIM3_IRQHandler(void);
void WEAK TIM4_IRQHandler(void);
void WEAK I2C1_EV_IRQHandler(void);
void WEAK I2C1_ER_IRQHandler(void);
void WEAK I2C2_EV_IRQHandler(void);
void WEAK I2C2_ER_IRQHandler(void);
void WEAK SPI1_IRQHandler(void);
void WEAK SPI2_IRQHandler(void);
void WEAK USART1_IRQHandler(void);
void WEAK USART2_IRQHandler(void);
void WEAK USART3_IRQHandler(void);
void WEAK EXTI15_10_IRQHandler(void);
void WEAK RTC_Alarm_IRQHandler(void);
void WEAK OTG_FS_WKUP_IRQHandler(void);
void WEAK TIM8_BRK_TIM12_IRQHandler(void);
void WEAK TIM8_UP_TIM13_IRQHandler(void);
void WEAK TIM8_TRG_COM_TIM14_IRQHandler(void);
void WEAK TIM8_CC_IRQHandler(void);
void WEAK DMA1_Stream7_IRQHandler(void);
void WEAK FSMC_IRQHandler(void);
void WEAK SDIO_IRQHandler(void);
void WEAK TIM5_IRQHandler(void);
void WEAK SPI3_IRQHandler(void);
void WEAK UART4_IRQHandler(void);
void WEAK UART5_IRQHandler(void);
void WEAK TIM6_DAC_IRQHandler(void);
void WEAK TIM7_IRQHandler(void);
void WEAK DMA2_Stream0_IRQHandler(void);
void WEAK DMA2_Stream1_IRQHandler(void);
void WEAK DMA2_Stream2_IRQHandler(void);
void WEAK DMA2_Stream3_IRQHandler(void);
void WEAK DMA2_Stream4_IRQHandler(void);
void WEAK CAN2_TX_IRQHandler(void);
void WEAK CAN2_RX0_IRQHandler(void);
void WEAK CAN2_RX1_IRQHandler(void);
void WEAK CAN2_SCE_IRQHandler(void);
void WEAK OTG_FS_IRQHandler(void);
void WEAK DMA2_Stream5_IRQHandler(void);
void WEAK DMA2_Stream6_IRQHandler(void);
void WEAK DMA2_Stream7_IRQHandler(void);
void WEAK USART6_IRQHandler(void);
void WEAK I2C3_EV_IRQHandler(void);
void WEAK I2C3_ER_IRQHandler(void);
void WEAK OTG_HS_EP1_OUT_IRQHandler(void);
void WEAK OTG_HS_EP1_IN_IRQHandler(void);
void WEAK OTG_HS_WKUP_IRQHandler(void);
void WEAK OTG_HS_IRQHandler(void);
void WEAK RNG_IRQHandler(void);
void WEAK FPU_IRQHandler(void);

__attribute__((section(".isr_vector"), used, aligned(512)))
void (* const g_pfnVectors[])(void) =
{
    (void (*)(void))&__stack,
    Reset_Handler,
    NMI_Handler,
    HardFault_Handler,
    MemManage_Handler,
    BusFault_Handler,
    UsageFault_Handler,
    0,
    0,
    0,
    0,
    SVC_Handler,
    DebugMon_Handler,
    0,
    PendSV_Handler,
    SysTick_Handler,

    /* External Interrupts */
    WWDG_IRQHandler,                   /* Window WatchDog */
    PVD_IRQHandler,                    /* PVD through EXTI Line detection */
    TAMP_STAMP_IRQHandler,             /* Tamper and TimeStamps through the EXTI line */
    RTC_WKUP_IRQHandler,               /* RTC Wakeup through the EXTI line */
    FLASH_IRQHandler,                  /* FLASH */
    RCC_IRQHandler,                    /* RCC */
    EXTI0_IRQHandler,                  /* EXTI Line0 */
    EXTI1_IRQHandler,                  /* EXTI Line1 */
    EXTI2_IRQHandler,                  /* EXTI Line2 */
    EXTI3_IRQHandler,                  /* EXTI Line3 */
    EXTI4_IRQHandler,                  /* EXTI Line4 */
    DMA1_Stream0_IRQHandler,           /* DMA1 Stream 0 */
    DMA1_Stream1_IRQHandler,           /* DMA1 Stream 1 */
    DMA1_Stream2_IRQHandler,           /* DMA1 Stream 2 */
    DMA1_Stream3_IRQHandler,           /* DMA1 Stream 3 */
    DMA1_Stream4_IRQHandler,           /* DMA1 Stream 4 */
    DMA1_Stream5_IRQHandler,           /* DMA1 Stream 5 */
    DMA1_Stream6_IRQHandler,           /* DMA1 Stream 6 */
    ADC_IRQHandler,                    /* ADC1, ADC2 and ADC3s */
    CAN1_TX_IRQHandler,                /* CAN1 TX */
    CAN1_RX0_IRQHandler,               /* CAN1 RX0 */
    CAN1_RX1_IRQHandler,               /* CAN1 RX1 */
    CAN1_SCE_IRQHandler,               /* CAN1 SCE */
    EXTI9_5_IRQHandler,                /* External Line[9:5]s */
    TIM1_BRK_TIM9_IRQHandler,          /* TIM1 Break and TIM9 */
    TIM1_UP_TIM10_IRQHandler,          /* TIM1 Update and TIM10 */
    TIM1_TRG_COM_TIM11_IRQHandler,     /* TIM1 Trigger and Commutation and TIM11 */
    TIM1_CC_IRQHandler,                /* TIM1 Capture Compare */
    TIM2_IRQHandler,                   /* TIM2 */
    TIM3_IRQHandler,                   /* TIM3 */
    TIM4_IRQHandler,                   /* TIM4 */
    I2C1_EV_IRQHandler,                /* I2C1 Event */
    I2C1_ER_IRQHandler,                /* I2C1 Error */
    I2C2_EV_IRQHandler,                /* I2C2 Event */
    I2C2_ER_IRQHandler,                /* I2C2 Error */
    SPI1_IRQHandler,                   /* SPI1 */
    SPI2_IRQHandler,                   /* SPI2 */
    USART1_IRQHandler,                 /* USART1 */
    USART2_IRQHandler,                 /* USART2 */
    USART3_IRQHandler,                 /* USART3 */
    EXTI15_10_IRQHandler,              /* External Line[15:10]s */
    RTC_Alarm_IRQHandler,              /* RTC Alarm (A and B) through EXTI Line */
    OTG_FS_WKUP_IRQHandler,            /* USB OTG FS Wakeup through EXTI line */
    TIM8_BRK_TIM12_IRQHandler,         /* TIM8 Break and TIM12 */
    TIM8_UP_TIM13_IRQHandler,          /* TIM8 Update and TIM13 */
    TIM8_TRG_COM_TIM14_IRQHandler,     /* TIM8 Trigger and Commutation and TIM14 */
    TIM8_CC_IRQHandler,                /* TIM8 Capture Compare */
    DMA1_Stream7_IRQHandler,           /* DMA1 Stream7 */
    FSMC_IRQHandler,                   /* FSMC */
    SDIO_IRQHandler,                   /* SDIO */
    TIM5_IRQHandler,                   /* TIM5 */
    SPI3_IRQHandler,                   /* SPI3 */
    UART4_IRQHandler,                  /* UART4 */
    UART5_IRQHandler,                  /* UART5 */
    TIM6_DAC_IRQHandler,               /* TIM6 and DAC1&2 underrun errors */
    TIM7_IRQHandler,                   /* TIM7 */
    DMA2_Stream0_IRQHandler,           /* DMA2 Stream 0 */
    DMA2_Stream1_IRQHandler,           /* DMA2 Stream 1 */
    DMA2_Stream2_IRQHandler,           /* DMA2 Stream 2 */
    DMA2_Stream3_IRQHandler,           /* DMA2 Stream 3 */
    DMA2_Stream4_IRQHandler,           /* DMA2 Stream 4 */
    0,                                 /* Reserved */
    0,                                 /* Reserved */
    CAN2_TX_IRQHandler,                /* CAN2 TX */
    CAN2_RX0_IRQHandler,               /* CAN2 RX0 */
    CAN2_RX1_IRQHandler,               /* CAN2 RX1 */
    CAN2_SCE_IRQHandler,               /* CAN2 SCE */
    OTG_FS_IRQHandler,                 /* USB OTG FS */
    DMA2_Stream5_IRQHandler,           /* DMA2 Stream 5 */
    DMA2_Stream6_IRQHandler,           /* DMA2 Stream 6 */
    DMA2_Stream7_IRQHandler,           /* DMA2 Stream 7 */
    USART6_IRQHandler,                 /* USART6 */
    I2C3_EV_IRQHandler,                /* I2C3 event */
    I2C3_ER_IRQHandler,                /* I2C3 error */
    OTG_HS_EP1_OUT_IRQHandler,         /* USB OTG HS End Point 1 Out */
    OTG_HS_EP1_IN_IRQHandler,          /* USB OTG HS End Point 1 In */
    OTG_HS_WKUP_IRQHandler,            /* USB OTG HS Wakeup through EXTI */
    OTG_HS_IRQHandler,                 /* USB OTG HS */
    0,                                 /* Reserved */
    0,                                 /* Reserved */
    RNG_IRQHandler,                    /* Hash and Rng */
    FPU_IRQHandler                     /* FPU */
};


_Static_assert((sizeof(g_pfnVectors) / sizeof(g_pfnVectors[0])) == 98U,
               "Unexpected STM32F405 vector-table length");


void Default_Handler(void)
{
    for (;;)
    {
        __NOP();
    }
}


void Reset_Handler(void)
{
    const uint32_t *source;
    uint32_t *destination;

    /* Copy .data segment to RAM */
    source = &__data_source;
    destination = &__data_start;
    while (destination < &__data_end)
    {
        *destination = *source;
        ++destination;
        ++source;
    }

    /* Copy .ccmram segment to CCM RAM */
    source = &__ccmram_source;
    destination = &__ccmram_start;
    while (destination < &__ccmram_end)
    {
        *destination = *source;
        ++destination;
        ++source;
    }

    /* Zero out .bss segment */
    destination = &__bss_start;
    while (destination < &__bss_end)
    {
        *destination = 0U;
        ++destination;
    }

    /* Zero out .ccmbss segment */
    destination = &__ccmram_bss_start;
    while (destination < &__ccmram_bss_end)
    {
        *destination = 0U;
        ++destination;
    }

    /* Copy .tdata segment to RAM */
    source = &__tdata_source;
    destination = &__tls_base;
    while (destination < &__tdata_end)
    {
        *destination = *source;
        ++destination;
        ++source;
    }

    /* Zero out .tbss segment */
    destination = &__tdata_end;
    while (destination < &__tls_end)
    {
        *destination = 0U;
        ++destination;
    }

    /* Initialize Picolibc TLS for the main thread */
    _set_tls(&__tls_base);

    /* Initialize system and start application */
    SystemInit();
    __libc_init_array();
    (void)main();

    for (;;)
    {
        __NOP();
    }

}
