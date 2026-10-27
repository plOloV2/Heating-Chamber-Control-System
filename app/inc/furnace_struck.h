#ifndef FURNACE_STRUCTS_H
#define FURNACE_STRUCTS_H

/**
 * @file furnace_structs.h
 * @brief Definition of main data struct cotroling baking.
 *
 * @details
 * This file defines most important data structure, which stores baking stages info.
 *
 * @author Aleksander Dziągwa
 * @date 20.10.2026
 */

#include <stdint.h>
#include "mem_location.h"


/** @brief Max number of stages in one baking process. */
#define MAX_STAGES 8U

/**
 * @name furnace_stage_data
 * @brief Structure that holds information about individual baking stages.
 */
typedef struct {
    /** @brief Stage duration in minutes. */
    uint16_t stage_time_min;

    /** @brief Target temperature in Celsius. 0xFF indicates an error or unset state. */
    uint8_t stage_required_temp;

    /** 
     * @brief Temperature growth rate in degrees per minute.
     * Values are typically between 0.0 and 10.0. 
     * Divide this integer value by 10 to obtain the actual decimal value.
     */
    uint8_t temp_grow;

    /** @brief Boolean flag; true if the fan is active during this stage. */
    bool FAN_ON;

    /** @brief Name of the stage for LCD display purposes (max 32 chars). */
    char stage_name[32];
} furnace_stage_data;

extern furnace_stage_data Stages[MAX_STAGES];
_Static_assert(sizeof(Stages) / sizeof(furnace_stage_data) == MAX_STAGES, "Unexpected furnace_stage_data[] size.");


//--------------------------------------------------------------------------------------------------------------------------------------------


// /**
//  * @brief Structure for PT1000 sensor data and processing.
//  */
// typedef struct {
//     /** @brief Bitmask of active sensors for error rejection algorithms. */
//     uint8_t active_sensors;

//     /** @brief Raw ADC values measured from four PT1000 sensors. */
//     uint32_t raw_val_PT1000[4];

//     /** @brief Converted temperature values in Celsius. */
//     float temp_celsius_PT1000[4];
// } temp_PT1000;


// /**
//  * @brief Structure for tracking baking duration and stage progress.
//  */
// typedef struct {
//     /** @brief Current second count. */
//     uint8_t seconds;

//     /** @brief Total minutes elapsed since the baking process started. */
//     uint16_t minutes_since_start;

//     /** @brief Minutes elapsed since the current stage began. */
//     uint16_t minutes_since_stage;
// } baking_time;

// /**
//  * @brief Main system data structure.
//  * Contains all sub-structures, thread IDs, and synchronization primitives.
//  */
// typedef struct {

//     /** @brief Index of the current active stage. */
//     uint8_t curr_stages;

//     /** 
//      * @brief Current operational state of the furnace.
//      * 0xff -> error ocured while calculating state,
//      * heating elements and fan are off.
//      */
//     uint8_t furnace_state;

//     /** @brief Current system error code (0 indicates no error). */
//     uint16_t error_code;

//     /* --- FreeRTOS Semaphores --- */
//     osSemaphoreId_t stage_sem; /**< Semaphore for furnace_stage_data access. */
//     osSemaphoreId_t temp_sem;  /**< Semaphore for temp_PT1000 access. */
//     osSemaphoreId_t time_sem;  /**< Semaphore for baking_time access. */
//     osSemaphoreId_t joke_sem;  /**< Semaphore for joke buffer access. */
//     osSemaphoreId_t errc_sem;  /**< Semaphore for error_code access. */

//     /* --- FreeRTOS Thread IDs --- */
//     osThreadId_t time_id;
//     osThreadId_t temp_id;
//     osThreadId_t oven_id;
//     osThreadId_t SD_id;
//     osThreadId_t LCD_id;
//     osThreadId_t BUZZER_id;

// } furnace_state;

#endif // DATA_STRUCTS_H_
