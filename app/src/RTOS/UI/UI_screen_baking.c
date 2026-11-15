/**
 * @file UI_screen_baking.c
 * @brief Live baking dashboard rendering (gauges, progress bar, clock, sprites).
 *
 * @author Aleksander Dziągwa
 * @date 15.11.2026
 */

#include "RTOS/UI/UI_screens.h"
#include "RTOS/temp_task.h"

#include "EVE4_Riverdi_lib/screen_platform.h"
#include "EVE4_Riverdi_lib/App_Common.h"

#include "drivers/RTC_operations.h"
#include "furnace_struck.h"
#include <stdio.h>
#include <stdlib.h>

extern bool heater_on;
extern bool fan_on;


void UI_RenderBakingScreen(Gpu_Hal_Context_t *phost, bool flash_ok)
{
    Gpu_CoCmd_Dlstart(phost);

    App_WrCoCmd_Buffer(phost, CLEAR_COLOR_RGB(150, 150, 150));
    App_WrCoCmd_Buffer(phost, CLEAR(1, 1, 1));

    App_WrCoCmd_Buffer(phost, COLOR_RGB(0, 0, 0));
    App_WrCoCmd_Buffer(phost, VERTEX_FORMAT(0U));

    /** Draw divider lines for bottom status bar */
    App_WrCoCmd_Buffer(phost, BEGIN(LINES));
    App_WrCoCmd_Buffer(phost, VERTEX2F(0, 188));
    App_WrCoCmd_Buffer(phost, VERTEX2F(480, 188));
    App_WrCoCmd_Buffer(phost, VERTEX2F(80, 188));
    App_WrCoCmd_Buffer(phost, VERTEX2F(80, 272));
    App_WrCoCmd_Buffer(phost, VERTEX2F(160, 188));
    App_WrCoCmd_Buffer(phost, VERTEX2F(160, 272));
    App_WrCoCmd_Buffer(phost, VERTEX2F(380, 188));
    App_WrCoCmd_Buffer(phost, VERTEX2F(380, 272));
    App_WrCoCmd_Buffer(phost, END());

    /** Draw 4 gauges for temperature readings */
    const float temp1 = TempReal[0];
    const float temp2 = TempReal[1];
    const float temp3 = TempReal[2];
    const float temp4 = TempReal[3];

    App_WrCoCmd_Buffer(phost, COLOR_RGB(255, 16, 72));
    Gpu_CoCmd_BgColor(phost, 0x222222);

    Gpu_CoCmd_Gauge(phost, 60,  88, 36, OPT_FLAT, 4, 8, (uint16_t)(temp1 + 30.0f), 230);
    Gpu_CoCmd_Gauge(phost, 180, 88, 36, OPT_FLAT, 4, 8, (uint16_t)(temp2 + 30.0f), 230);
    Gpu_CoCmd_Gauge(phost, 300, 88, 36, OPT_FLAT, 4, 8, (uint16_t)(temp3 + 30.0f), 230);
    Gpu_CoCmd_Gauge(phost, 420, 88, 36, OPT_FLAT, 4, 8, (uint16_t)(temp4 + 30.0f), 230);

    /** Draw stage progress bar */
    const uint16_t min_passed = Furnace_state.minutes_passed;
    const uint16_t min_all    = Stages[Furnace_state.stage_ID].stage_time_min;

    App_WrCoCmd_Buffer(phost, COLOR_RGB(255, 158, 73));
    Gpu_CoCmd_Progress(phost, 40, 172, 400, 12, OPT_FLAT, min_passed, min_all);

    /** Draw clock in bottom-right corner */
    const uint32_t time = read_time();
    const uint16_t hour = RTC_UNPACK_HOUR(time);
    const uint16_t min  = RTC_UNPACK_MIN(time);
    const uint16_t sec  = RTC_UNPACK_SEC(time);

    App_WrCoCmd_Buffer(phost, COLOR_RGB(255, 226, 6));
    Gpu_CoCmd_Clock(phost, 430, 238, 32, OPT_FLAT, hour, min, sec, 0);

    /** Draw labels and text readouts */
    App_WrCoCmd_Buffer(phost, COLOR_RGB(0, 0, 0));
    Gpu_CoCmd_Text(phost, 240, 0,  30, OPT_CENTERX, "BAKING");
    Gpu_CoCmd_Text(phost, 60,  36, 26, OPT_CENTERX, "Temp 1");
    Gpu_CoCmd_Text(phost, 180, 36, 26, OPT_CENTERX, "Temp 2");
    Gpu_CoCmd_Text(phost, 300, 36, 26, OPT_CENTERX, "Temp 3");
    Gpu_CoCmd_Text(phost, 420, 36, 26, OPT_CENTERX, "Temp 4");

    char temp_val1[8], temp_val2[8], temp_val3[8], temp_val4[8];
    strfromf(temp_val1, sizeof(temp_val1), "%.1f", temp1);
    strfromf(temp_val2, sizeof(temp_val2), "%.1f", temp2);
    strfromf(temp_val3, sizeof(temp_val3), "%.1f", temp3);
    strfromf(temp_val4, sizeof(temp_val4), "%.1f", temp4);

    Gpu_CoCmd_Text(phost, 60,  128, 26, OPT_CENTERX, temp_val1);
    Gpu_CoCmd_Text(phost, 180, 128, 26, OPT_CENTERX, temp_val2);
    Gpu_CoCmd_Text(phost, 300, 128, 26, OPT_CENTERX, temp_val3);
    Gpu_CoCmd_Text(phost, 420, 128, 26, OPT_CENTERX, temp_val4);

    char prog_bar_text[64];
    snprintf(prog_bar_text, sizeof(prog_bar_text), "%u out of %u minutes passed", min_passed, min_all);
    Gpu_CoCmd_Text(phost, 240, 153, 27, OPT_CENTERX, prog_bar_text);

    Gpu_CoCmd_Text(phost, 40,  188, 27, OPT_CENTERX, heater_on ? "Heat ON" : "Heat OFF");
    Gpu_CoCmd_Text(phost, 120, 188, 27, OPT_CENTERX, fan_on    ? "Fan ON"  : "Fan OFF");

    char stage_info[128];
    const float    avg_tempt = Furnace_state.average_temp;
    const uint32_t cur_stage = Furnace_state.stage_ID;
    const uint32_t all_stage = Furnace_state.stages_num;
    const uint32_t tem_stage = Stages[cur_stage].stage_required_temp;
    const int32_t  gro_stage = Stages[cur_stage].temp_grow;
    const uint32_t tim_stage = Stages[cur_stage].stage_time_min;

    snprintf(stage_info, sizeof(stage_info),
             "Stage %u out of %u.\nStage temp: %u\nAverage temp: %.1f\nTemp groth: %d\nStage lenght: %u",
             (unsigned)(cur_stage + 1U), (unsigned)all_stage, (unsigned)tem_stage,
             (double)avg_tempt, (int)gro_stage, (unsigned)tim_stage);
    Gpu_CoCmd_Text(phost, 164, 190, 26, 0, stage_info);

    char time_str[12];
    snprintf(time_str, sizeof(time_str), "%02u:%02u:%02u", hour, min, sec);
    Gpu_CoCmd_Text(phost, 430, 188, 27, OPT_CENTERX, time_str);

    if(flash_ok)
    {
        App_WrCoCmd_Buffer(phost, COLOR_RGB(255, 255, 255));

        UI_LoadOneAsset(phost, flash_ok, HEAT_OFFSET, heater_on ? Heat_on_64x64_ASTC_4X4_raw : Heat_off_64x64_ASTC_4X4_raw);
        App_WrCoCmd_Buffer(phost, BITMAP_HANDLE(2U));
        Gpu_CoCmd_SetBitmap(phost, HEAT_OFFSET, COMPRESSED_RGBA_ASTC_4x4_KHR, 64, 64);
        App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS));
        App_WrCoCmd_Buffer(phost, VERTEX2F(4, 208));
        App_WrCoCmd_Buffer(phost, END());

        UI_LoadOneAsset(phost, flash_ok, FAN_OFFSET, fan_on ? Fan_rot_0_64x64_ASTC_4X4_raw : Fan_off_64x64_ASTC_4X4_raw);
        App_WrCoCmd_Buffer(phost, BITMAP_HANDLE(3U));
        Gpu_CoCmd_SetBitmap(phost, FAN_OFFSET, COMPRESSED_RGBA_ASTC_4x4_KHR, 64, 64);
        App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS));
        App_WrCoCmd_Buffer(phost, VERTEX2F(88, 208));
        App_WrCoCmd_Buffer(phost, END());
    }

    App_WrCoCmd_Buffer(phost, DISPLAY());
    Gpu_CoCmd_Swap(phost);
    App_Flush_Co_Buffer(phost);
    Gpu_Hal_WaitCmdfifo_empty(phost);
}
