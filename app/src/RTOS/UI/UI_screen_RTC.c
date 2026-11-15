/**
 * @file UI_screen_RTC.c
 * @brief RTC date and time setup screen logic and rendering.
 *
 * @author Aleksander Dziągwa
 * @date 15.11.2026
 */

#include "RTOS/UI/UI_screens.h"

#include "EVE4_Riverdi_lib/screen_platform.h"
#include "EVE4_Riverdi_lib/App_Common.h"

#include "drivers/buttons_operations.h"
#include "drivers/RTC_operations.h"
#include <stdio.h>

typedef enum {
    RTC_FIELD_YEAR = 0U,
    RTC_FIELD_MONTH,
    RTC_FIELD_DAY,
    RTC_FIELD_HOUR,
    RTC_FIELD_MIN,
    RTC_FIELD_SEC,
    RTC_FIELD_CONFIRM
} rtc_edit_field_t;

typedef struct {
    uint16_t x;
    uint16_t up_y;
    uint16_t down_y;
} rtc_arrow_pos_t;

static const rtc_arrow_pos_t rtc_arrow_coords[RTC_FIELD_CONFIRM] = {
    [RTC_FIELD_YEAR]  = { .x = 200U, .up_y = 40U, .down_y = 92U  },
    [RTC_FIELD_MONTH] = { .x = 240U, .up_y = 40U, .down_y = 92U  },
    [RTC_FIELD_DAY]   = { .x = 270U, .up_y = 40U, .down_y = 92U  },
    [RTC_FIELD_HOUR]  = { .x = 210U, .up_y = 98U, .down_y = 150U },
    [RTC_FIELD_MIN]   = { .x = 240U, .up_y = 98U, .down_y = 150U },
    [RTC_FIELD_SEC]   = { .x = 270U, .up_y = 98U, .down_y = 150U }
};

static bool             rtc_screen_init = false;
static rtc_edit_field_t current_field   = RTC_FIELD_YEAR;
static uint16_t         edit_year       = 2000U;
static uint8_t          edit_month      = 1U;
static uint8_t          edit_day        = 1U;
static uint8_t          edit_hour       = 0U;
static uint8_t          edit_min        = 0U;
static uint8_t          edit_sec        = 0U;

/** @brief Returns number of days in given month. */
static uint8_t rtc_get_days_in_month(uint16_t year, uint8_t month)
{
    static const uint8_t days_table[12] = {
        31U, 28U, 31U, 30U, 31U, 30U, 31U, 31U, 30U, 31U, 30U, 31U
    };

    if((month < 1U) || (month > 12U))
    {
        return 31U;
    }

    if((month == 2U) && ((year % 4U) == 0U))
    {
        return 29U;
    }

    return days_table[month - 1U];
}


bool UI_UpdateRtcSetState(void)
{
    if(!rtc_screen_init)
    {
        current_field   = RTC_FIELD_YEAR;
        rtc_screen_init = true;
    }

    const button_event_t btn = ButtonState;
    ButtonStateChanged = false;

    if(btn.pressed.but4) /* RIGHT */
    {
        current_field = (current_field < RTC_FIELD_CONFIRM) ? (current_field + 1U) : RTC_FIELD_YEAR;
    }
    else if(btn.pressed.but3) /* LEFT */
    {
        current_field = (current_field > RTC_FIELD_YEAR) ? (current_field - 1U) : RTC_FIELD_CONFIRM;
    }
    else if(btn.pressed.but1 || btn.pressed.but2) /* UP or DOWN */
    {
        const bool is_up = (btn.pressed.but1 != 0U);

        switch(current_field)
        {
            case RTC_FIELD_YEAR:
                edit_year = is_up ? ((edit_year < 2099U) ? (edit_year + 1U) : 2000U)
                                  : ((edit_year > 2000U) ? (edit_year - 1U) : 2099U);
                break;

            case RTC_FIELD_MONTH:
                edit_month = is_up ? ((edit_month < 12U) ? (edit_month + 1U) : 1U)
                                   : ((edit_month > 1U)  ? (edit_month - 1U) : 12U);
                break;

            case RTC_FIELD_DAY:
            {
                const uint8_t max_day = rtc_get_days_in_month(edit_year, edit_month);
                edit_day = is_up ? ((edit_day < max_day) ? (edit_day + 1U) : 1U)
                                 : ((edit_day > 1U)      ? (edit_day - 1U) : max_day);
                break;
            }

            case RTC_FIELD_HOUR:
                edit_hour = is_up ? ((edit_hour < 23U) ? (edit_hour + 1U) : 0U)
                                  : ((edit_hour > 0U)  ? (edit_hour - 1U) : 23U);
                break;

            case RTC_FIELD_MIN:
                edit_min = is_up ? ((edit_min < 59U) ? (edit_min + 1U) : 0U)
                                 : ((edit_min > 0U)  ? (edit_min - 1U) : 59U);
                break;

            case RTC_FIELD_SEC:
                edit_sec = is_up ? ((edit_sec < 59U) ? (edit_sec + 1U) : 0U)
                                 : ((edit_sec > 0U)  ? (edit_sec - 1U) : 59U);
                break;

            case RTC_FIELD_CONFIRM:
                if(is_up && set_date_time(edit_year, edit_month, edit_day, edit_hour, edit_min, edit_sec))
                {
                    RTC_valid       = true;
                    rtc_screen_init = false;
                    return true;
                }
                break;

            default:
                break;
        }

        const uint8_t max_valid_day = rtc_get_days_in_month(edit_year, edit_month);
        if(edit_day > max_valid_day)
        {
            edit_day = max_valid_day;
        }
    }

    return false;
}


void UI_RenderRtcSetScreen(Gpu_Hal_Context_t *phost, bool flash_ok)
{
    Gpu_CoCmd_Dlstart(phost);
    App_WrCoCmd_Buffer(phost, CLEAR_COLOR_RGB(150, 150, 150));
    App_WrCoCmd_Buffer(phost, CLEAR(1, 1, 1));
    App_WrCoCmd_Buffer(phost, VERTEX_FORMAT(0U));

    App_WrCoCmd_Buffer(phost, COLOR_RGB(225, 0, 0));
    Gpu_CoCmd_Text(phost, 20, 16, 26, 0, "The RTC clock is not initialized. Please enter current date and time.");

    char date_str[24];
    char time_str[24];
    snprintf(date_str, sizeof(date_str), "Date: %04u.%02u.%02u", edit_year, edit_month, edit_day);
    snprintf(time_str, sizeof(time_str), "Time: %02u:%02u:%02u", edit_hour, edit_min, edit_sec);

    App_WrCoCmd_Buffer(phost, COLOR_RGB(0, 0, 0));
    Gpu_CoCmd_Text(phost, 134, 66, 28, 0, date_str);
    Gpu_CoCmd_Text(phost, 150, 124, 28, 0, time_str);
    Gpu_CoCmd_Text(phost, 20, 230, 26, 0, "Use UP and DOWN arrows to change values,\nRIGHT to move forward, LEFT backward.");

    if(current_field == RTC_FIELD_CONFIRM)
    {
        App_WrCoCmd_Buffer(phost, COLOR_RGB(59, 223, 0));
    }
    else
    {
        App_WrCoCmd_Buffer(phost, COLOR_RGB(60, 60, 60));
    }
    Gpu_CoCmd_Text(phost, 190, 170, 28, 0, "Set RTC");

    if(flash_ok && (current_field < RTC_FIELD_CONFIRM))
    {
        const uint16_t arrow_x      = rtc_arrow_coords[current_field].x;
        const uint16_t arrow_up_y   = rtc_arrow_coords[current_field].up_y;
        const uint16_t arrow_down_y = rtc_arrow_coords[current_field].down_y;

        App_WrCoCmd_Buffer(phost, COLOR_RGB(255, 255, 255));

        Gpu_CoCmd_LoadIdentity(phost);
        Gpu_CoCmd_RotateAround(phost, 12, 12, 16384, 65536);
        Gpu_CoCmd_SetMatrix(phost);

        App_WrCoCmd_Buffer(phost, BITMAP_HANDLE(3U));
        Gpu_CoCmd_SetBitmap(phost, ARROW_OFFSET, COMPRESSED_RGBA_ASTC_4x4_KHR, 24, 24);
        App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS));
        App_WrCoCmd_Buffer(phost, VERTEX2F(arrow_x, arrow_down_y));
        App_WrCoCmd_Buffer(phost, END());

        Gpu_CoCmd_LoadIdentity(phost);
        Gpu_CoCmd_RotateAround(phost, 12, 12, 49152, 65536);
        Gpu_CoCmd_SetMatrix(phost);

        App_WrCoCmd_Buffer(phost, BITMAP_HANDLE(4U));
        Gpu_CoCmd_SetBitmap(phost, ARROW_OFFSET, COMPRESSED_RGBA_ASTC_4x4_KHR, 24, 24);
        App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS));
        App_WrCoCmd_Buffer(phost, VERTEX2F(arrow_x, arrow_up_y));
        App_WrCoCmd_Buffer(phost, END());

        Gpu_CoCmd_LoadIdentity(phost);
        Gpu_CoCmd_SetMatrix(phost);
    }

    App_WrCoCmd_Buffer(phost, DISPLAY());
    Gpu_CoCmd_Swap(phost);
    App_Flush_Co_Buffer(phost);
    Gpu_Hal_WaitCmdfifo_empty(phost);
}
