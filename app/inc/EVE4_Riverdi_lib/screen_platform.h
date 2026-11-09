/*
 * Copyright (c) Riverdi Sp. z o.o. sp. k. <riverdi@riverdi.com>
 * Copyright (c) Skalski Embedded Technologies <contact@lukasz-skalski.com>
 */

#ifndef _PLATFORM_H_
#define _PLATFORM_H_

/* C library inclusions */
#include <stdarg.h>
#include <string.h>

/* STM32 library inclusions */
#include "stm32f4xx.h"

/*****************************************************************************/

/* type definitions for EVE HAL library */

#define TRUE		(1)
#define FALSE		(0)

typedef char		    bool_t;
typedef char		    char8_t;
typedef unsigned char   uchar8_t;
typedef signed char	    schar8_t;
typedef float		    float_t;

/* Predefined Riverdi modules */
#include "modules.h"

/* EVE inclusions */
#include "Gpu_Hal.h"
#include "Gpu.h"
#include "CoPro_Cmds.h"
#include "Hal_Utils.h"

/* Custom flash map */
#include "screen_flash_map.h"

/* Custom G RAM locations */
#define BACKGROUND_OFFSET   0U          // size: 130560
#define LOGO_OFFSET         130560U     // size: 33792
#define ARROW_OFFSET        164352U     // size: 1024
#define FAN_OFFSET          165376U     // size: 4096
#define HEAT_OFFSET         169472U     // size: 4096
#define KUBICA_OFFSET       173568U     // size: 4096

/*****************************************************************************/

typedef enum {
  GPIO_CS   = 0,
  GPIO_PD   = 1,
  GPIO_INT  = 2
} gpio_name;

typedef enum {
  GPIO_LOW  = 0,
  GPIO_HIGH = 1
} gpio_val;

/*****************************************************************************/

bool_t platform_init (Gpu_HalInit_t*);
void platform_sleep_ms (uint32_t);

bool_t platform_spi_init (Gpu_Hal_Context_t*);
void platform_spi_deinit (Gpu_Hal_Context_t*);

uchar8_t platform_spi_send_recv_byte (Gpu_Hal_Context_t*, uchar8_t, uint32_t);
uint16_t platform_spi_send_data (Gpu_Hal_Context_t*, uchar8_t*, uint16_t, uint32_t);
void platform_spi_recv_data (Gpu_Hal_Context_t*, uchar8_t*, uint16_t, uint32_t);

bool_t platform_gpio_init (Gpu_Hal_Context_t*, gpio_name);
bool_t platform_gpio_value (Gpu_Hal_Context_t*, gpio_name, gpio_val);

/*****************************************************************************/

#endif /*_PLATFORM_H_*/
