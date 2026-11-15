/**
 * @file UI_screen_setup.c
 * @brief Multi-stage baking setup screen logic and rendering.
 *
 * @author Aleksander Dziągwa
 * @date 15.11.2026
 */

#include "RTOS/UI/UI_screens.h"

#include "EVE4_Riverdi_lib/screen_platform.h"
#include "EVE4_Riverdi_lib/App_Common.h"

#include "drivers/buttons_operations.h"
#include "furnace_struck.h"
#include <stdio.h>
#include <stdlib.h>

typedef enum {
    SETUP_FIELD_TEMP = 0U,
    SETUP_FIELD_GROW,
    SETUP_FIELD_DURATION,
    SETUP_FIELD_FAN,
    SETUP_FIELD_LAST_STAGE,
    SETUP_FIELD_NEXT
} setup_edit_field_t;

static bool               setup_screen_init = false;
static setup_edit_field_t current_field     = SETUP_FIELD_TEMP;
static uint8_t            edit_stage        = 0U;
static uint8_t            edit_temp         = 50U;
static int8_t             edit_grow         = 10;
static uint16_t           edit_duration     = 60U;
static bool               edit_fan          = true;
static bool               edit_last_stage   = false;


bool UI_UpdateSetupBakingState(void)
{
    if(!setup_screen_init)
    {
        edit_stage        = 0U;
        edit_temp         = 80U;
        edit_grow         = 10;
        edit_duration     = 60U;
        edit_fan          = false;
        edit_last_stage   = false;
        current_field     = SETUP_FIELD_TEMP;
        setup_screen_init = true;
    }

    if(edit_stage >= (MAX_STAGES - 1U))
    {
        edit_last_stage = true;
    }

    const button_event_t btn = ButtonState;
    ButtonStateChanged = false;

    if(btn.pressed.but4) /* RIGHT */
    {
        current_field = (current_field < SETUP_FIELD_NEXT) ? (current_field + 1U) : SETUP_FIELD_TEMP;
    }
    else if(btn.pressed.but3) /* LEFT */
    {
        current_field = (current_field > SETUP_FIELD_TEMP) ? (current_field - 1U) : SETUP_FIELD_NEXT;
    }
    else if(btn.pressed.but1 || btn.pressed.but2) /* UP or DOWN */
    {
        const bool is_up = (btn.pressed.but1 != 0U);

        switch(current_field)
        {
            case SETUP_FIELD_TEMP:
                edit_temp = is_up ? ((edit_temp < 200U) ? (edit_temp + 5U) : 20U)
                                  : ((edit_temp > 20U)  ? (edit_temp - 5U) : 200U);
                break;

            case SETUP_FIELD_GROW:
                edit_grow = is_up ? ((edit_grow < 50) ? (edit_grow + 1) : 0)
                                  : ((edit_grow > 0)  ? (edit_grow - 1) : 50);
                break;

            case SETUP_FIELD_DURATION:
                edit_duration = is_up ? ((edit_duration < 999U) ? (edit_duration + 5U) : 5U)
                                      : ((edit_duration > 5U)   ? (edit_duration - 5U) : 999U);
                break;

            case SETUP_FIELD_FAN:
                edit_fan = !edit_fan;
                break;

            case SETUP_FIELD_LAST_STAGE:
                edit_last_stage = (edit_stage < (MAX_STAGES - 1U)) ? !edit_last_stage : true;
                break;

            case SETUP_FIELD_NEXT:
                if(is_up)
                {
                    Stages[edit_stage].stage_required_temp = edit_temp;
                    Stages[edit_stage].temp_grow           = edit_grow;
                    Stages[edit_stage].stage_time_min      = edit_duration;
                    Stages[edit_stage].FAN_ON              = edit_fan;

                    if(edit_last_stage || (edit_stage >= (MAX_STAGES - 1U)))
                    {
                        Furnace_state.stages_num     = edit_stage + 1U;
                        Furnace_state.stage_ID       = 0U;
                        Furnace_state.minutes_passed = 0U;
                        Furnace_state.seconds_passed = 0U;

                        setup_screen_init = false;
                        return true;
                    }

                    edit_stage++;
                    edit_last_stage = (edit_stage >= (MAX_STAGES - 1U));
                    current_field   = SETUP_FIELD_TEMP;
                }
                break;

            default:
                break;
        }
    }

    return false;
}


void UI_RenderSetupBakingScreen(Gpu_Hal_Context_t *phost)
{
    Gpu_CoCmd_Dlstart(phost);
    App_WrCoCmd_Buffer(phost, CLEAR_COLOR_RGB(150, 150, 150));
    App_WrCoCmd_Buffer(phost, CLEAR(1, 1, 1));
    App_WrCoCmd_Buffer(phost, VERTEX_FORMAT(0U));

    char stage_title_str[16];
    char temp_str[16];
    char grow_str[16];
    char duration_str[16];

    snprintf(stage_title_str, sizeof(stage_title_str), "Stage %u", (unsigned)(edit_stage + 1U));
    snprintf(temp_str,        sizeof(temp_str),        "%u C",     (unsigned)edit_temp);
    snprintf(grow_str,        sizeof(grow_str),        "%d.%d C",  edit_grow / 10, abs(edit_grow % 10));
    snprintf(duration_str,    sizeof(duration_str),    "%u min",   (unsigned)edit_duration);

    App_WrCoCmd_Buffer(phost, COLOR_RGB(0, 0, 0));
    Gpu_CoCmd_Text(phost, 240, 0,   30, OPT_CENTERX, stage_title_str);
    Gpu_CoCmd_Text(phost, 240, 60,  27, OPT_RIGHTX,  "Temp: ");
    Gpu_CoCmd_Text(phost, 240, 90,  27, OPT_RIGHTX,  "Temp grow: ");
    Gpu_CoCmd_Text(phost, 240, 120, 27, OPT_RIGHTX,  "Duration: ");
    Gpu_CoCmd_Text(phost, 240, 150, 27, OPT_RIGHTX,  "FAN: ");
    Gpu_CoCmd_Text(phost, 240, 180, 27, OPT_RIGHTX,  "Last stage? : ");

    App_WrCoCmd_Buffer(phost, (current_field == SETUP_FIELD_TEMP) ? COLOR_RGB(236, 145, 19) : COLOR_RGB(0, 0, 0));
    Gpu_CoCmd_Text(phost, 240, 60, 27, 0, temp_str);

    App_WrCoCmd_Buffer(phost, (current_field == SETUP_FIELD_GROW) ? COLOR_RGB(236, 145, 19) : COLOR_RGB(0, 0, 0));
    Gpu_CoCmd_Text(phost, 240, 90, 27, 0, grow_str);

    App_WrCoCmd_Buffer(phost, (current_field == SETUP_FIELD_DURATION) ? COLOR_RGB(236, 145, 19) : COLOR_RGB(0, 0, 0));
    Gpu_CoCmd_Text(phost, 240, 120, 27, 0, duration_str);

    App_WrCoCmd_Buffer(phost, (current_field == SETUP_FIELD_FAN) ? COLOR_RGB(236, 145, 19) : COLOR_RGB(0, 0, 0));
    Gpu_CoCmd_Text(phost, 240, 150, 27, 0, edit_fan ? "ON" : "OFF");

    App_WrCoCmd_Buffer(phost, (current_field == SETUP_FIELD_LAST_STAGE) ? COLOR_RGB(236, 145, 19) : COLOR_RGB(0, 0, 0));
    Gpu_CoCmd_Text(phost, 240, 180, 27, 0, edit_last_stage ? "YES" : "NO");

    App_WrCoCmd_Buffer(phost, (current_field == SETUP_FIELD_NEXT) ? COLOR_RGB(236, 145, 19) : COLOR_RGB(0, 0, 0));
    Gpu_CoCmd_Text(phost, 240, 230, 29, OPT_CENTERX, "NEXT");

    App_WrCoCmd_Buffer(phost, DISPLAY());
    Gpu_CoCmd_Swap(phost);
    App_Flush_Co_Buffer(phost);
    Gpu_Hal_WaitCmdfifo_empty(phost);
}
