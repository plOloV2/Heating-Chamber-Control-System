/**
 * @file buzzer_task.c
 * @brief Implementation of buzzer RTOS task controlling passive buzzer via TIM2's PWM.
 *
 * @author Aleksander Dziągwa
 * @date 23.10.2026
 */

#include "drivers/buzzer_operations.h"
#include "system_state/error.h"
#include "RTOS/buzzer_task.h"
#include "furnace_struck.h"
#include "FreeRTOS.h"
#include "task.h"

/** @brief Number of error melodies in lookup table. */
#define ERROR_MELODIES_COUNT    6U

/** @brief Signal played after succesful system initialization (SystemStatus == NO_ERROR). */
const buzzer_note_t ready_to_bake[BUZZER_MELODY_NOTES_COUNT] = {
    { 1046U, 400U },
    {  880U, 400U },
    { 1046U, 400U },
    {  880U, 400U },
    { 1046U, 400U },
    {  880U, 400U },
    {    0U,   0U },
    {    0U,   0U }
};

/** @brief Signal played after core system initialization fail (SystemStatus == SYSTEM_INIT_ERROR). */
const buzzer_note_t core_fail[BUZZER_MELODY_NOTES_COUNT] = {
    {  880U, 1000U},
    {    0U,   0U },
    {    0U,   0U },
    {    0U,   0U },
    {    0U,   0U },
    {    0U,   0U },
    {    0U,   0U },
    {    0U,   0U }
};

/** @brief Signal played after RTC system fail (SystemStatus == RTC_DATE_ERROR). */
const buzzer_note_t RTC_fail[BUZZER_MELODY_NOTES_COUNT] = {
    {  880U, 500U },
    {    0U, 500U },
    {  880U, 500U },
    {    0U,   0U },
    {    0U,   0U },
    {    0U,   0U },
    {    0U,   0U },
    {    0U,   0U }
};

/** @brief Signal played after LCD fail (SystemStatus == LCD_INIT_ERROR). */
const buzzer_note_t LCD_fail[BUZZER_MELODY_NOTES_COUNT] = {
    {  880U, 500U },
    {    0U, 500U },
    {  880U, 250U },
    {    0U, 500U },
    {  880U, 500U },
    {    0U,   0U },
    {    0U,   0U },
    {    0U,   0U }
};

/** @brief Signal played after SD fail (SystemStatus == SD_CARD_INIT_ERROR). */
const buzzer_note_t SD_init_fail[BUZZER_MELODY_NOTES_COUNT] = {
    {  880U, 500U },
    {    0U, 500U },
    {  880U, 500U },
    {    0U, 500U },
    {  880U, 500U },
    {    0U, 500U },
    {  880U, 500U },
    {    0U,   0U }
};

/** @brief Signal played after RTOS fail (SystemStatus == RTOS_ERROR). */
const buzzer_note_t SD_read_fail[BUZZER_MELODY_NOTES_COUNT] = {
    {  880U, 500U },
    {    0U, 500U },
    {  880U, 250U },
    {    0U, 250U },
    {  880U, 250U },
    {    0U, 250U },
    {  880U, 500U },
    {    0U,   0U }
};

/** @brief Signal played after RTOS fail (SystemStatus == RTOS_ERROR). */
const buzzer_note_t SD_write_fail[BUZZER_MELODY_NOTES_COUNT] = {
    {  880U, 500U },
    {    0U, 500U },
    {  880U, 250U },
    {    0U, 250U },
    {  880U, 500U },
    {    0U, 500U },
    {  880U, 250U },
    {    0U,   0U }
};

/** @brief Signal played after stage has finished (SystemStatus == ). */
const buzzer_note_t stage_done[BUZZER_MELODY_NOTES_COUNT] = {
    {  659U, 500U },
    {    0U, 500U },
    {  659U, 500U },
    {    0U,   0U },
    {    0U,   0U },
    {    0U,   0U },
    {    0U,   0U },
    {    0U,   0U }
};

/** @brief Signal played after baking has finished (SystemStatus == ). */
const buzzer_note_t baking_done[BUZZER_MELODY_NOTES_COUNT] = {
    {  523U, 120U },
    {    0U,  20U },
    {  587U, 120U },
    {    0U,  20U },
    {  659U, 120U },
    {    0U,  20U },
    {  698U, 120U },
    {    0U,  20U }
};

/** @brief Look up table for melodies. */
const buzzer_note_t * const melodies[ERROR_MELODIES_COUNT] = {
    core_fail,
    RTC_fail,
    LCD_fail,
    SD_init_fail,
    SD_read_fail,
    SD_write_fail
};

static void play_melody(const buzzer_note_t melody[])
{

    if(melody == nullptr)
    {
        return;
    }

    for (uint32_t i = 0; i < BUZZER_MELODY_NOTES_COUNT; i++)
    {
        uint32_t freq = melody[i].frequency_hz;
        uint32_t dur = melody[i].duration_ms;

        if(dur == 0U)
        {
            break;
        }

        buzzer_play_tone(freq);
        vTaskDelay(pdMS_TO_TICKS(dur));

    }

    /* Ensure buzzer is turned off after melody ends */
    buzzer_play_tone(0U);

}

static void check_error(void)
{
    /** Bitmask of error flags that have already been signaled */
    static uint32_t signaled_errors = 0U;

    /** Isolate only error bits that have not been played yet */
    const uint32_t new_errors = SystemStatus & ~signaled_errors;

    if(new_errors == 0U)
    {
        return;
    }

    for(size_t i = 0U; i < ERROR_MELODIES_COUNT; i++)
    {
        const uint32_t error_mask = (1UL << i);

        if((new_errors & error_mask) != 0U)
        {
            /** Latch bit so this error will not be signaled again */
            signaled_errors |= error_mask;

            play_melody(melodies[i]);
            vTaskDelay(pdMS_TO_TICKS(2500U));
        }

    }

}

/** @brief Checks if a baking stage or the entire baking process has ende. */
static void check_stage_end(void)
{
    static uint8_t last_stage_id       = 0U;
    static bool    baking_done_played  = false;

    const uint8_t stages_num = Furnace_state.stages_num;
    const uint8_t stage_id   = Furnace_state.stage_ID;

    /** Do nothing until user finishes configuring stages in SETUP_BAKING */
    if(stages_num == 0U)
    {
        last_stage_id      = 0U;
        baking_done_played = false;
        return;
    }

    /** Detect if stage_ID has advanced since last 250ms check */
    if(stage_id != last_stage_id)
    {
        last_stage_id = stage_id;

        if(stage_id >= stages_num)
        {
            /** Final stage finished: play baking_done once */
            if(!baking_done_played)
            {
                baking_done_played = true;
                play_melody(baking_done);
            }

        }
        else
        {
            /** Intermediate stage finished: play stage_done */
            play_melody(stage_done);
        }

    }

}


/** @brief RTOS task body for buzzer control. */
void vBuzzerTask(void *pvParameters)
{
    (void)pvParameters;

    buzzer_hw_init();
    buzzer_play_tone(0U);

    /** Play startup melody if no boot errors occurred */
    if(SystemStatus == NO_ERROR)
    {
        play_melody(ready_to_bake);
    }

    for(;;)
    {
        check_error();
        check_stage_end();

        vTaskDelay(pdMS_TO_TICKS(250U));
    }

}
