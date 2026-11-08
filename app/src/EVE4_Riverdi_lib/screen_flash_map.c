#include "EVE4_Riverdi_lib/screen_flash_map.h"

/* Flash asset list array */
eve_flash_asset_size_info flash_asset_list[flash_asset_index_last] =
{
    [default_fl_blob] = {
        .offset = 0,
        .size = 4096
    },
    [Arrow_big_32x32_ASTC_4X4_raw] = {
        .offset = 4096,
        .size = 1024
    },
    [Arrow_medium_24x24_ASTC_4X4_raw] = {
        .offset = 5120,
        .size = 576
    },
    [Arrow_small_16x16_ASTC_4X4_raw] = {
        .offset = 5696,
        .size = 256
    },
    [Fan_off_64x64_ASTC_4X4_raw] = {
        .offset = 5952,
        .size = 4096
    },
    [Fan_rot_0_64x64_ASTC_4X4_raw] = {
        .offset = 10048,
        .size = 4096
    },
    [Fan_rot_1_64x64_ASTC_4X4_raw] = {
        .offset = 14144,
        .size = 4096
    },
    [Fan_rot_2_64x64_ASTC_4X4_raw] = {
        .offset = 18240,
        .size = 4096
    },
    [Farewell_480x272_ASTC_4X4_raw] = {
        .offset = 22336,
        .size = 130560
    },
    [Giga_Kubica_64x52_ASTC_4X4_raw] = {
        .offset = 152896,
        .size = 3328
    },
    [Heat_off_64x64_ASTC_4X4_raw] = {
        .offset = 156224,
        .size = 4096
    },
    [Heat_on_64x64_ASTC_4X4_raw] = {
        .offset = 160320,
        .size = 4096
    },
    [Klatwa_480x272_ASTC_4X4_raw] = {
        .offset = 164416,
        .size = 130560
    },
    [Logo_PWR_RT_BLACK_384x88_ASTC_4X4_raw] = {
        .offset = 294976,
        .size = 33792
    },
    [Logo_PWR_RT_GOLD_384x88_ASTC_4X4_raw] = {
        .offset = 328768,
        .size = 33792
    },
    [Logo_PWR_RT_WHITE_384x88_ASTC_4X4_raw] = {
        .offset = 362560,
        .size = 33792
    },
    [Oven_fire_480x272_ASTC_4X4_raw] = {
        .offset = 396352,
        .size = 130560
    },
    [RT14e_480x272_ASTC_4X4_raw] = {
        .offset = 526912,
        .size = 130560
    },
    [RT15e_1_480x272_ASTC_4X4_raw] = {
        .offset = 657472,
        .size = 130560
    },
    [RT15e_2_480x272_ASTC_4X4_raw] = {
        .offset = 788032,
        .size = 130560
    },
    [RT15e_3_480x272_ASTC_4X4_raw] = {
        .offset = 918592,
        .size = 130560
    },
    [RT15e_4_480x272_ASTC_4X4_raw] = {
        .offset = 1049152,
        .size = 130560
    },
    [RT15e_5_480x272_ASTC_4X4_raw] = {
        .offset = 1179712,
        .size = 130560
    },
    [RT15e_6_480x272_ASTC_4X4_raw] = {
        .offset = 1310272,
        .size = 130560
    },
    [RT15e_7_480x272_ASTC_4X4_raw] = {
        .offset = 1440832,
        .size = 130560
    },
    [RT15e_8_480x272_ASTC_4X4_raw] = {
        .offset = 1571392,
        .size = 130560
    },
    [RT15e_9_480x272_ASTC_4X4_raw] = {
        .offset = 1701952,
        .size = 130560
    },
    [RT15e_10_480x272_ASTC_4X4_raw] = {
        .offset = 1832512,
        .size = 130560
    },
    [RT15e_11_480x272_ASTC_4X4_raw] = {
        .offset = 1963072,
        .size = 130560
    },
    [RT15e_13_480x272_ASTC_4X4_raw] = {
        .offset = 2093632,
        .size = 130560
    },
    [RT15e_14_480x272_ASTC_4X4_raw] = {
        .offset = 2224192,
        .size = 130560
    },
    [RT16e_1_480x272_ASTC_4X4_raw] = {
        .offset = 2354752,
        .size = 130560
    },
    [RT16e_2_480x272_ASTC_4X4_raw] = {
        .offset = 2485312,
        .size = 130560
    },
    [RT16e_3_480x272_ASTC_4X4_raw] = {
        .offset = 2615872,
        .size = 130560
    },
    [magic_val_bin] = {
        .offset = 2746432,
        .size = 2
    }
};

/* Total flash asset size */
// Can be read using extern declaration
const uint32_t total_flash_asset = flash_asset_index_last;
