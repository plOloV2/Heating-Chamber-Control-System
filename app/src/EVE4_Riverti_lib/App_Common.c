/*

 Copyright (c) Bridgetek Pte Ltd
 Copyright (c) Riverdi Sp. z o.o. sp. k.
 Copyright (c) Lukasz Skalski <contact@lukasz-skalski.com>

 THIS SOFTWARE IS PROVIDED BY BRIDGETEK PTE LTD "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 BRIDGETEK PTE LTD BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 OF SUBSTITUTE GOODS OR SERVICES LOSS OF USE, DATA, OR PROFITS OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 BRIDGETEK DRIVERS MAY BE USED ONLY IN CONJUNCTION WITH PRODUCTS BASED ON BRIDGETEK PARTS.

 BRIDGETEK DRIVERS MAY BE DISTRIBUTED IN ANY FORM AS LONG AS LICENSE INFORMATION IS NOT MODIFIED.

 IF A CUSTOM VENDOR ID AND/OR PRODUCT ID OR DESCRIPTION STRING ARE USED, IT IS THE
 RESPONSIBILITY OF THE PRODUCT MANUFACTURER TO MAINTAIN ANY CHANGES AND SUBSEQUENT WHQL
 RE-CERTIFICATION AS A RESULT OF MAKING THESE CHANGES.

 Abstract:

 Application to demonstrate function of EVE.

 Author : Bridgetek

 Revision History:
 0.1 - date 2017.03.24 - Initial version
 */

#include "EVE4_Riverti_lib/platform.h"
#include "EVE4_Riverti_lib/App_Common.h"

/*************************************************************************************************
 *                      These functions work with FT8XX GPU buffer
 **************************************************************************************************/

uint32_t CmdBuffer_Index;
volatile uint32_t DlBuffer_Index;


void App_WrCoCmd_Buffer(Gpu_Hal_Context_t *phost, uint32_t cmd) {

	Gpu_Hal_WrCmd32(phost, cmd);

	/* Increment the command index */
	CmdBuffer_Index += CMD_SIZE;
}

void App_WrDl_Buffer(Gpu_Hal_Context_t *phost, uint32_t cmd) {

	Gpu_Hal_Wr32(phost, (RAM_DL + DlBuffer_Index), cmd);

	/* Increment the command index */
	DlBuffer_Index += CMD_SIZE;
}


void App_Flush_DL_Buffer(Gpu_Hal_Context_t *phost) {
	DlBuffer_Index = 0;
}

void App_Flush_Co_Buffer(Gpu_Hal_Context_t *phost) {
	CmdBuffer_Index = 0;
}

void App_Flush_Co_Buffer_nowait(Gpu_Hal_Context_t *phost) {
	CmdBuffer_Index = 0;
}

void App_Set_DlBuffer_Index(uint32_t index) {
	DlBuffer_Index = index;
}

void App_Set_CmdBuffer_Index(uint32_t index) {
	CmdBuffer_Index = index;
}
/*************************************************************************************************
 *                      Application Utilities
 **************************************************************************************************/

static uint8_t sk = 0;
uint8_t App_Read_Tag(Gpu_Hal_Context_t *phost) {
	static uint8_t Read_tag = 0, temp_tag = 0, ret_tag = 0;
	Read_tag = Gpu_Hal_Rd8(phost, REG_TOUCH_TAG);
	ret_tag = 0;
	if (Read_tag != 0)                           // Allow if the Key is released
			{
		if (temp_tag != Read_tag) {
			temp_tag = Read_tag;
			sk = Read_tag;                 // Load the Read tag to temp variable
		}
	} else {
		if (temp_tag != 0) {
			ret_tag = temp_tag;
			temp_tag = 0; //The event will be processed. Clear the tag
		}
		sk = 0;
	}
	return ret_tag;
}

uint8_t App_Touch_Update(Gpu_Hal_Context_t *phost, uint8_t *currTag,
		uint16_t *x, uint16_t *y) {
	static uint8_t Read_tag = 0, temp_tag = 0, ret_tag = 0;
	uint32_t touch;
	Read_tag = Gpu_Hal_Rd8(phost, REG_TOUCH_TAG);
	ret_tag = 0;
	if (Read_tag != 0)                           // Allow if the Key is released
			{
		ret_tag = 1;
		if (temp_tag != Read_tag) {
			temp_tag = Read_tag;
			sk = Read_tag;                 // Load the Read tag to temp variable
		}
	} else {
		if (temp_tag != 0) {
			ret_tag = temp_tag;
			temp_tag = 0; //The event will be processed. Clear the tag
		}
		sk = 0;
	}
	*currTag = Read_tag;
	touch = Gpu_Hal_Rd32(phost, REG_TOUCH_SCREEN_XY);
	*x = (uint16_t) (touch >> 16);
	*y = (uint16_t) (touch & 0xFFFF);
	return ret_tag;
}
void App_Play_Sound(Gpu_Hal_Context_t *phost, uint8_t sound, uint8_t vol,
		uint8_t midi) {
	uint16_t val = (midi << 8) | sound;
	Gpu_Hal_Wr8(phost, REG_VOL_SOUND, vol);
	Gpu_Hal_Wr16(phost, REG_SOUND, val);
	Gpu_Hal_Wr8(phost, REG_PLAY, 1);
}

void App_Calibrate_Screen(Gpu_Hal_Context_t *phost) {
	Gpu_CoCmd_Dlstart(phost);
	App_WrCoCmd_Buffer(phost, CLEAR(1, 1, 1));
	App_WrCoCmd_Buffer(phost, COLOR_RGB(0, 0, 0));
	for (int i = 0; i < 16; i++)
		Gpu_CoCmd_Text(phost, DispWidth / 2, 20 + 20 * i, 16 + i,
				OPT_CENTERX | OPT_CENTERY, "Please tap on a dot °C");

	/* Calibration */
	Gpu_CoCmd_Calibrate(phost, 0);
	App_Flush_Co_Buffer(phost);
	Gpu_Hal_WaitCmdfifo_empty(phost);
}



/******************************************************************************/

void App_Common_Init(Gpu_Hal_Context_t *phost) {
	Gpu_HalInit_t halinit;
	uint8_t chipid;

	Gpu_Hal_Init(&halinit);
	Gpu_Hal_Open(phost);

	Gpu_Hal_Powercycle(phost, TRUE);

	/* FT81x will be in SPI Single channel after POR
	 If we are here with FT4222 in multi channel, then
	 an explicit switch to single channel is essential
	 */

	Gpu_Hal_SetSPI(phost, GPU_SPI_SINGLE_CHANNEL, GPU_SPI_ONEDUMMY);

	Gpu_HostCommand(phost,GPU_EXTERNAL_OSC);
	Gpu_81X_SelectSysCLK(phost, GPU_SYSCLK_72M); /* Select 72 MHz PLL while in SLEEP mode */

	Gpu_Hal_Sleep(100);
	Gpu_HostCommand(phost, GPU_ACTIVE_M);
	Gpu_Hal_Sleep(300);

	/* read Register ID to check if chip ID series is correct */
	chipid = Gpu_Hal_Rd8(phost, REG_ID);
	while (chipid != 0x7C) {
		chipid = Gpu_Hal_Rd8(phost, REG_ID);
		Gpu_Hal_Sleep(100);
	}

	/* read REG_CPURESET to confirm 0 is returned */
	{
		uint8_t engine_status;

		/* Read REG_CPURESET to check if engines are ready.
		 Bit 0 for coprocessor engine,
		 Bit 1 for touch engine,
		 Bit 2 for audio engine.
		 */
		engine_status = Gpu_Hal_Rd8(phost, REG_CPURESET);
		while (engine_status != 0x00) {
			engine_status = Gpu_Hal_Rd8(phost, REG_CPURESET);
			Gpu_Hal_Sleep(100);
		}
	}

	Gpu_Hal_Wr32(phost, REG_FREQUENCY, 72000000);


	Gpu_Hal_Wr16(phost, REG_PWM_HZ, 4000);
	Gpu_Hal_Wr8(phost, REG_PWM_DUTY, 128);

	Gpu_Hal_Wr16(phost, REG_PCLK_FREQ, DispPLCLKFREQ);
	Gpu_Hal_Wr8(phost, REG_PCLK_2X, DispPCLK2x);


	/* configuration of LCD display */
	Gpu_Hal_Wr16(phost, REG_HCYCLE, DispHCycle);
	Gpu_Hal_Wr16(phost, REG_HOFFSET, DispHOffset);
	Gpu_Hal_Wr16(phost, REG_HSYNC0, DispHSync0);
	Gpu_Hal_Wr16(phost, REG_HSYNC1, DispHSync1);
	Gpu_Hal_Wr16(phost, REG_VCYCLE, DispVCycle);
	Gpu_Hal_Wr16(phost, REG_VOFFSET, DispVOffset);
	Gpu_Hal_Wr16(phost, REG_VSYNC0, DispVSync0);
	Gpu_Hal_Wr16(phost, REG_VSYNC1, DispVSync1);
	Gpu_Hal_Wr8(phost, REG_SWIZZLE, DispSwizzle);
	Gpu_Hal_Wr8(phost, REG_PCLK_POL, DispPCLKPol);
	Gpu_Hal_Wr16(phost, REG_HSIZE, DispWidth);
	Gpu_Hal_Wr16(phost, REG_VSIZE, DispHeight);
	Gpu_Hal_Wr16(phost, REG_CSPREAD, DispCSpread);
	Gpu_Hal_Wr16(phost, REG_DITHER, DispDither);

	/* GPIO configuration */
	Gpu_Hal_Wr16(phost, REG_GPIOX_DIR, 0xFFFF);
	Gpu_Hal_Wr16(phost, REG_GPIOX, 0xFFFF);


	Gpu_ClearScreen(phost);

	/* after this display is visible on the LCD */
	Gpu_Hal_Wr8(phost, REG_PCLK, DispPCLK);

	phost->cmd_fifo_wp = Gpu_Hal_Rd16(phost, REG_CMD_WRITE);
}

void App_Common_Close(Gpu_Hal_Context_t *phost) {
	Gpu_Hal_Close(phost);
	Gpu_Hal_DeInit();
}
