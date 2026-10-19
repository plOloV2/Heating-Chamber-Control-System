#ifndef ERROR_H
#define ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    NO_ERROR = 0U,
    SYSTEM_INIT_ERROR = 1U << 0,
    RTC_DATE_ERROR = 1U << 1,
    LCD_INIT_ERROR = 1U << 2,
    SD_CARD_INIT_ERROR = 1U << 3,
    RTOS_ERROR = 1U << 4
} system_error_t;

extern volatile system_error_t SystemStatus;

#ifdef __cplusplus
}
#endif

#endif
