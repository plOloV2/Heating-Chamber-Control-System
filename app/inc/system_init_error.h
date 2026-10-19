#ifndef SYSTEM_INIT_ERROR_H
#define SYSTEM_INIT_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SYSTEM_INIT_STATUS_OK = 0U,
    SYSTEM_INIT_STATUS_HSE_TIMEOUT = 1U << 0U,
    SYSTEM_INIT_STATUS_PLL_TIMEOUT = 1U << 1U,
    SYSTEM_INIT_STATUS_SYSCLK_SWITCH_TIMEOUT = 1U << 2U,
    SYSTEM_INIT_STATUS_LSE_TIMEOUT = 1U << 3U,
    SYSTEM_INIT_STATUS_GPIO_FAIL = 1U << 4U
} system_init_status_t;

extern volatile system_init_status_t SystemInitStatus;

#ifdef __cplusplus
}
#endif

#endif
