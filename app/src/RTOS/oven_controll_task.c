/**
 * @file oven_controll_task.c
 * @brief Heating and fan control task. Most important task in system.
 *
 * @details This task reads user programed curring parameters and real temperature
 *          from temp task. Controlls heating and fan outputs to match required temp.
 *          Oven controll task is waken up by temp task once every second.
 *
 * @author Aleksander Dziągwa
 * @date 12.11.2026
 */

#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"

#include "stm32f4xx.h"
#include "GPIO_pins_def.h"
#include "furnace_struck.h"
#include "RTOS/temp_task.h"
#include "drivers/pin_operations.h"
#include "drivers/IWDG_operations.h"
#include "RTOS/oven_controll_task.h"

/**
 * @brief Hysteresis & thermal inertia lead parameters
 * @details
 * - HYST_BAND_C:     Base half-band around target_temp (+/- 0.3 C)
 * - SLOPE_LEAD_SEC:  Looks ahead by N seconds based on current dT/dt (1s step)
 *                    so heater turns ON early when falling toward target_temp
 *                    and turns OFF early when rising toward target_temp.
 */
#define TEMP_HYST_BAND_C        0.3f
#define TEMP_SLOPE_LEAD_SEC     8.0f

bool fan_on = false;
bool heater_on = false;

/** @brief Static helpers */
static inline float GetAverageTemp(void);
static void UpdateCurrentState(float *p_target_temp, bool *p_first_run, float *p_prev_temp);
static void UpdateTargetTempOnMinute(float *p_target_temp, const furnace_stage_data *p_stage);
static void ControlFanAndHeater(float target_temp, float current_temp, float prev_temp);


void vOvenControllTask(void *pvParameters)
{
    (void)pvParameters;

    float target_temp = 0.0f;
    float prev_temp   = 0.0f;
    bool  first_run   = true;

    /** Wait to be waken up by UI task to start baking */
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    /** Start TIM7 1s continuous counter */
    SET_BIT(TIM7->CR1, TIM_CR1_CEN);

    /** Start 5s watchdog */
    configure_iwdg();

    while(true)
    {
        /** Wait to be resumed by temperature task (every 1 second) */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        /** Reload the watchdog */
        reload_iwdg();

        /** Update average temp, timers, stage transitions, and target_temp */
        UpdateCurrentState(&target_temp, &first_run, &prev_temp);

        /** Check if curing process has finished all stages */
        if(Furnace_state.stage_ID >= Furnace_state.stages_num)
        {
            /** Safely turn OFF Heater and Fan before exiting curing loop */
            SetPin(HEAT_PORT, HEAT_PIN, !HEAT_ACTIVE);
            SetPin(FAN_PORT,  FAN_PIN,  !FAN_ACTIVE);
            heater_on = false;
            first_run = true;
            break;
        }

        /** Drive FAN and HEAT pins with bidirectional slope-aware hysteresis */
        ControlFanAndHeater(target_temp, Furnace_state.average_temp, prev_temp);

        /** Drive pins acording to calculations */
        SetPin(FAN_PORT,  FAN_PIN,  (fan_on)    ? FAN_ACTIVE :  !FAN_ACTIVE);
        SetPin(HEAT_PORT, HEAT_PIN, (heater_on) ? HEAT_ACTIVE : !HEAT_ACTIVE);

        /** Save current temperature for next second's slope (dT/dt) calculation */
        prev_temp = Furnace_state.average_temp;
    }

    /** Suspend permanently if task exits curing loop */
    vTaskSuspend(NULL);
}


/** @brief Calculate mean temperature from all 4 PT1000 sensors */
static inline float GetAverageTemp(void)
{
    float average = 0.0f;

    average += TempReal[0];
    average += TempReal[1];
    average += TempReal[2];
    average += TempReal[3];

    return (average / 4.0f);
}


/** @brief Increment target_temp every minute if below stage_required_temp */
static void UpdateTargetTempOnMinute(float *p_target_temp, const furnace_stage_data *p_stage)
{
    const float stage_goal = (float)p_stage->stage_required_temp;

    /** 0.0 deg/min (temp_grow == 0) means fast uncontrolled heating directly to stage goal */
    if(p_stage->temp_grow == 0U)
    {
        *p_target_temp = stage_goal;
        return;
    }

    /** Ramp target_temp up every minute if it is still lower than stage's required temp */
    if(*p_target_temp < stage_goal)
    {
        const float step_deg_per_min = (float)p_stage->temp_grow / 10.0f;
        *p_target_temp += step_deg_per_min;

        /** Clamp to stage_required_temp so we never overshoot the programmed stage ceiling */
        if(*p_target_temp > stage_goal)
        {
            *p_target_temp = stage_goal;
        }

    }

}


/** @brief Updates average temperature, elapsed time, stage transitions, and target_temp */
static void UpdateCurrentState(float *p_target_temp, bool *p_first_run, float *p_prev_temp)
{
    /**Always refresh current average temperature first */
    Furnace_state.average_temp = GetAverageTemp();

    /** Initialize target_temp at the very start of curing (Stage 0, second 0) */
    if(*p_first_run)
    {
        *p_first_run = false;

        if(Furnace_state.stage_ID == 0U)
        {
            /** First stage in curing process starts from measured average temperature */
            *p_target_temp = Furnace_state.average_temp;
        }
        else
        {
            /** Resumed mid-process: start from previous stage's required temperature */
            *p_target_temp = (float)Stages[Furnace_state.stage_ID - 1U].stage_required_temp;
        }

        /** If first stage has temp_grow == 0 (fast heating), jump immediately to goal */
        if(Stages[Furnace_state.stage_ID].temp_grow == 0U)
        {
            *p_target_temp = (float)Stages[Furnace_state.stage_ID].stage_required_temp;
        }

        *p_prev_temp = Furnace_state.average_temp;

    }

    /** A second has passed, increment seconds counter */
    Furnace_state.seconds_passed++;

    /** Check if a full minute has passed */
    if(Furnace_state.seconds_passed >= 60U)
    {
        Furnace_state.seconds_passed = 0U;
        Furnace_state.minutes_passed++;

        const uint8_t current_id = Furnace_state.stage_ID;

        /** Check if current stage has reached its programmed duration */
        if(Furnace_state.minutes_passed >= Stages[current_id].stage_time_min)
        {
            Furnace_state.minutes_passed = 0U;
            Furnace_state.stage_ID++;

            /** If more stages remain, initialize target_temp for the new stage */
            if(Furnace_state.stage_ID < Furnace_state.stages_num)
            {
                /** New stage starts at previous stage's targeted temperature */
                *p_target_temp = (float)Stages[current_id].stage_required_temp;

                /** If new stage uses fast uncontrolled heating (temp_grow == 0), set goal now */
                if(Stages[Furnace_state.stage_ID].temp_grow == 0U)
                {
                    *p_target_temp = (float)Stages[Furnace_state.stage_ID].stage_required_temp;
                }

            }

        }
        else
        {
            /** Still in the same stage: update target_temp for the new minute */
            UpdateTargetTempOnMinute(p_target_temp, &Stages[current_id]);
        }

    }

}


/** @brief Drives FAN and HEAT outputs using bidirectional slope-compensated hysteresis */
static void ControlFanAndHeater(float target_temp, float current_temp, float prev_temp)
{
    const uint8_t stage_id = Furnace_state.stage_ID;

    /** Fan Control: Directly follows current stage's FAN_ON flag */
    fan_on = Stages[stage_id].FAN_ON;    

    /** Heater Safety Check: 0xFF indicates an error or unset stage temperature */
    if(Stages[stage_id].stage_required_temp == 0xFFU)
    {
        heater_on = false;
        SetPin(HEAT_PORT, HEAT_PIN, !HEAT_ACTIVE);
        return;
    }

    /**
     * Bidirectional Slope-Aware Hysteresis:
     *    temp_rate = dT/dt over the last 1 second.
     *    predicted_temp estimates where the oven temperature will be in TEMP_SLOPE_LEAD_SEC.
     *    - When falling (temp_rate < 0), predicted_temp < current_temp, triggering ON early
     *      just before actual temperature drops below target_temp.
     *    - When rising (temp_rate > 0), predicted_temp > current_temp, triggering OFF early
     *      just before actual temperature overshoots target_temp.
     */
    const float temp_rate      = current_temp - prev_temp;
    const float predicted_temp = current_temp + (temp_rate * TEMP_SLOPE_LEAD_SEC);

    if(heater_on)
    {
        /** Heater is currently ON: turn OFF if predicted or actual temp exceeds upper bound */
        if( (predicted_temp >= (target_temp + TEMP_HYST_BAND_C)) ||
            (current_temp   >= (target_temp + (TEMP_HYST_BAND_C * 2.0f))))
        {
            heater_on = false;
        }

    }
    else
    {
        /** Heater is currently OFF: turn ON if predicted or actual temp drops below lower bound */
        if( (predicted_temp <= (target_temp - TEMP_HYST_BAND_C)) ||
            (current_temp   <= (target_temp - (TEMP_HYST_BAND_C * 2.0f))))
        {
            heater_on = true;
        }

    }

}
