#ifndef FURNACE_STRUCTS_H
#define FURNACE_STRUCTS_H

/**
 * @file furnace_structs.h
 * @brief Definition of main data struct cotroling baking.
 *
 * @details This file defines most important data structure, which stores baking stages info.
 *
 * @author Aleksander Dziągwa
 * @date 20.10.2026
 */

#include <stdint.h>
#include "mem_location.h"


/** @brief Max number of stages in one baking process. */
#define MAX_STAGES 16U

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
     * Values are typically between 0.0 and 5.0, where 0.0 means fast uncontrolled heating.
     * Divide this integer value by 10 to obtain the actual decimal value.
     */
    int8_t temp_grow;

    /** @brief Boolean flag; true if the fan is active during this stage. */
    bool FAN_ON;
} furnace_stage_data;

extern CCMRAM_BSS furnace_stage_data Stages[MAX_STAGES];
_Static_assert(sizeof(Stages) / sizeof(furnace_stage_data) == MAX_STAGES, "Unexpected furnace_stage_data[] size.");


/**
 * @name furnace_current_info
 * @brief Structure that holds information about current stage id, time passed and average temp.
 */
typedef struct {
    /** @brief Value calculated from 4 PT1000 readings */
    float average_temp;

    /** @brief Time passed in current stage */
    uint16_t seconds_passed;
    uint16_t minutes_passed;

    /** @brief ID of current stage */
    uint8_t stage_ID;

    /** @brief Number of all stages in current process */
    uint8_t stages_num;

} furnace_current_info;

extern CCMRAM_BSS volatile furnace_current_info Furnace_state;

#endif
