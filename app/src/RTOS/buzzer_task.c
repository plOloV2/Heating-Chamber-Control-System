/**
 * @file buzzer_task.c
 * @brief Implementation of buzzer RTOS task controlling passive buzzer via PWM.
 *
 * @details
 *
 * @author Aleksander Dziągwa
 * @date 23.10.2026
 */

#include "drivers/buzzer_operations.h"
#include "system_state/error.h"
#include "RTOS/buzzer_task.h"
#include "mem_location.h"
#include "FreeRTOS.h"
#include "task.h"

/** @brief Signal played after succesful system initialization (SystemStatus == NO_ERROR). */
CCMRAM_DATA const buzzer_note_t ready_to_bake[BUZZER_MELODY_NOTES_COUNT] = {
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
CCMRAM_DATA const buzzer_note_t core_fail[BUZZER_MELODY_NOTES_COUNT] = {
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
CCMRAM_DATA const buzzer_note_t RTC_fail[BUZZER_MELODY_NOTES_COUNT] = {
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
CCMRAM_DATA const buzzer_note_t LCD_fail[BUZZER_MELODY_NOTES_COUNT] = {
    {  880U, 500U },
    {    0U, 500U },
    {  880U, 500U },
    {    0U, 500U },
    {  880U, 500U },
    {    0U,   0U },
    {    0U,   0U },
    {    0U,   0U }
};

/** @brief Signal played after SD fail (SystemStatus == SD_CARD_INIT_ERROR). */
CCMRAM_DATA const buzzer_note_t SD_fail[BUZZER_MELODY_NOTES_COUNT] = {
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
CCMRAM_DATA const buzzer_note_t RTOS_fail[BUZZER_MELODY_NOTES_COUNT] = {
    {  880U, 500U },
    {    0U, 500U },
    {  880U, 250U },
    {    0U, 250U },
    {  880U, 250U },
    {    0U, 250U },
    {  880U, 500U },
    {    0U,   0U }
};

/** @brief Signal played after stage has finished (SystemStatus == ). */
CCMRAM_DATA const buzzer_note_t stage_done[BUZZER_MELODY_NOTES_COUNT] = {
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
CCMRAM_DATA const buzzer_note_t baking_done[BUZZER_MELODY_NOTES_COUNT] = {
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
CCMRAM_DATA const buzzer_note_t * const melodies[] = {
    core_fail,
    RTC_fail,
    LCD_fail,
    SD_fail,
    RTOS_fail,
    stage_done,     //not yet implemented
    baking_done     //not yet implemented
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

    if(SystemStatus == NO_ERROR)
    {
        play_melody(ready_to_bake);
        return;
    }

    for(size_t i = 0; i < SYSTEM_STATES; i++){
        if((SystemStatus & (1U << i)) != 0U)
        {
            play_melody(melodies[i+3]);
            vTaskDelay(pdMS_TO_TICKS(2500));
        }

    }

}


/**
 * @brief RTOS task body for buzzer control.
 */
void vBuzzerTask(void *pvParameters)
{
    (void)pvParameters;

    buzzer_hw_init();
    buzzer_play_tone(0U);

    check_error();

    for (;;)
    {
        /* Suspend self until resumed */
        vTaskSuspend(NULL);

        check_error();

    }

}
