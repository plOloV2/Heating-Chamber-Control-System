#ifndef SCREEN_FLASH_MAP_H
#define SCREEN_FLASH_MAP_H

#include <stdint.h>

/* Flash asset info structure */
typedef struct {
    uint32_t offset;
    uint32_t size;
} eve_flash_asset_size_info;

/* Flash asset index enum */
typedef enum {
    default_fl_blob = 0,
    Arrow_big_32x32_ASTC_4X4_raw = 1,
    Arrow_medium_24x24_ASTC_4X4_raw = 2,
    Arrow_small_16x16_ASTC_4X4_raw = 3,
    Fan_off_64x64_ASTC_4X4_raw = 4,
    Fan_rot_0_64x64_ASTC_4X4_raw = 5,
    Fan_rot_1_64x64_ASTC_4X4_raw = 6,
    Fan_rot_2_64x64_ASTC_4X4_raw = 7,
    Farewell_480x272_ASTC_4X4_raw = 8,
    Giga_Kubica_64x52_ASTC_4X4_raw = 9,
    Heat_off_64x64_ASTC_4X4_raw = 10,
    Heat_on_64x64_ASTC_4X4_raw = 11,
    Klatwa_480x272_ASTC_4X4_raw = 12,
    Logo_PWR_RT_BLACK_384x88_ASTC_4X4_raw = 13,
    Logo_PWR_RT_GOLD_384x88_ASTC_4X4_raw = 14,
    Logo_PWR_RT_WHITE_384x88_ASTC_4X4_raw = 15,
    Oven_fire_480x272_ASTC_4X4_raw = 16,
    RT14e_480x272_ASTC_4X4_raw = 17,
    RT15e_1_480x272_ASTC_4X4_raw = 18,
    RT15e_2_480x272_ASTC_4X4_raw = 19,
    RT15e_3_480x272_ASTC_4X4_raw = 20,
    RT15e_4_480x272_ASTC_4X4_raw = 21,
    RT15e_5_480x272_ASTC_4X4_raw = 22,
    RT15e_6_480x272_ASTC_4X4_raw = 23,
    RT15e_7_480x272_ASTC_4X4_raw = 24,
    RT15e_8_480x272_ASTC_4X4_raw = 25,
    RT15e_9_480x272_ASTC_4X4_raw = 26,
    RT15e_10_480x272_ASTC_4X4_raw = 27,
    RT15e_11_480x272_ASTC_4X4_raw = 28,
    RT15e_13_480x272_ASTC_4X4_raw = 29,
    RT15e_14_480x272_ASTC_4X4_raw = 30,
    RT16e_1_480x272_ASTC_4X4_raw = 31,
    RT16e_2_480x272_ASTC_4X4_raw = 32,
    RT16e_3_480x272_ASTC_4X4_raw = 33,
    magic_val_bin = 34,
    flash_asset_index_last
} flash_asset_index;

/* Flash list and it's size */
extern eve_flash_asset_size_info flash_asset_list[flash_asset_index_last];
extern const uint32_t total_flash_asset;

#endif /* SCREEN_FLASH_MAP_H */
