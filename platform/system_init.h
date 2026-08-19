#ifndef SYSTEM_INIT_H
#define SYSTEM_INIT_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef HSI_VALUE
    #define HSI_VALUE 16'000'000UL
#endif

#ifndef HSE_VALUE
    #define HSE_VALUE 16'000'000UL
#endif

#ifndef LSE_VALUE
    #define LSE_VALUE 32'768UL
#endif

#ifndef SYSTEM_CLOCK_HZ
    #define SYSTEM_CLOCK_HZ 168'000'000UL
#endif

typedef enum {
    SYSTEM_INIT_STATUS_OK = 0U,
    SYSTEM_INIT_STATUS_HSE_TIMEOUT = 1U << 0U,
    SYSTEM_INIT_STATUS_PLL_TIMEOUT = 1U << 1U,
    SYSTEM_INIT_STATUS_SYSCLK_SWITCH_TIMEOUT = 1U << 2U,
    SYSTEM_INIT_STATUS_LSE_TIMEOUT = 1U << 3U
} system_init_status_t;

/* Set by SystemInit() before the C/C++ runtime invokes main(). */
extern volatile system_init_status_t SystemInitStatus;

#ifdef __cplusplus
}
#endif

#endif
