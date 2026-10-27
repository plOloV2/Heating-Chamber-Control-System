/**
 * @file SD_task.c
 * @brief Implementation of SD RTOS task controlling SDIO interface (SD Card).
 *
 * @details
 *
 * @author Aleksander Dziągwa
 * @date 27.10.2026
 */

#include "system_state/error.h"
#include "RTOS/SD_task.h"
#include "mem_location.h"
#include "stm32f4xx.h"
#include "FreeRTOS.h"
#include "task.h"

#include "diskio.h"
#include "ff.h"

#define DISK_NUM 0U

void vSDTask(void *pvParameters){

    (void)pvParameters;

    DSTATUS SD_Card_init = disk_initialize(DISK_NUM);

    if(SD_Card_init != 0)
    {
        SET_BIT(SystemStatus, SD_CARD_INIT_ERROR);
    }

    vTaskSuspend(NULL);

    for(;;)
    {

        vTaskSuspend(NULL);

    }

}
