/**
 * @file UI_screen_end.c
 * @brief RTC date and time setup screen logic and rendering.
 *
 * @author Aleksander Dziągwa
 * @date 15.11.2026
 */

#include "RTOS/UI/UI_screens.h"

#include "EVE4_Riverdi_lib/screen_platform.h"
#include "EVE4_Riverdi_lib/App_Common.h"

void UI_RenderEndBakingScreen(Gpu_Hal_Context_t *phost, bool flash_ok)
{
    Gpu_CoCmd_Dlstart(phost);
    App_WrCoCmd_Buffer(phost, CLEAR_COLOR_RGB(150, 150, 150));
    App_WrCoCmd_Buffer(phost, CLEAR(1, 1, 1));
    App_WrCoCmd_Buffer(phost, VERTEX_FORMAT(0U));

    if(flash_ok)
    {
        UI_LoadOneAsset(phost, flash_ok, BACKGROUND_OFFSET, Farewell_480x272_ASTC_4X4_raw);
        App_WrCoCmd_Buffer(phost, BITMAP_HANDLE(0U));
        Gpu_CoCmd_SetBitmap(phost, BACKGROUND_OFFSET, COMPRESSED_RGBA_ASTC_4x4_KHR, 480, 272);
        App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS));
        App_WrCoCmd_Buffer(phost, VERTEX2F(0U, 0U));
        App_WrCoCmd_Buffer(phost, END());
    }

    App_WrCoCmd_Buffer(phost, COLOR_RGB(255, 158, 73));
    Gpu_CoCmd_Text(phost, 240, 10, 30, OPT_CENTERX, "Baking done!");

    App_WrCoCmd_Buffer(phost, DISPLAY());
    Gpu_CoCmd_Swap(phost);
    App_Flush_Co_Buffer(phost);
    Gpu_Hal_WaitCmdfifo_empty(phost);
}
