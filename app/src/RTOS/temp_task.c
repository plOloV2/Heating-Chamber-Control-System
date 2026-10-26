#include <stdint.h>
#include "mem_location.h"
#include "FreeRTOS.h"
#include "task.h"

#define LUT_START_TEMP  -30
#define LUT_END_TEMP    200
#define LUT_STEP        5
#define LUT_SIZE ((LUT_END_TEMP - LUT_START_TEMP) / LUT_STEP + 1)
const float LUT_ADC[LUT_SIZE] = {
    870.879,        // -30 C
    943.809,        // -25 C
    1014.873,       // -20 C
    1084.071,       // -15 C
    1151.543,       // -10 C
    1217.287,       // -5 C
    1281.433,       // 0 C
    1344.006,       // 5 C
    1405.064,       // 10 C
    1464.630,       // 15 C
    1522.847,       // 20 C
    1579.674,       // 25 C
    1635.187,       // 30 C
    1689.431,       // 35 C
    1742.476,       // 40 C
    1794.309,       // 45 C
    1845.022,       // 50 C
    1894.625,       // 55 C
    1943.155,       // 60 C
    1990.646,       // 65 C
    2037.130,       // 70 C
    2082.639,       // 75 C
    2127.226,       // 80 C
    2170.874,       // 85 C
    2213.657,       // 90 C
    2255.577,       // 95 C
    2296.639,       // 100 C
    2336.911,       // 105 C
    2376.415,       // 110 C
    2415.134,       // 115 C
    2453.108,       // 120 C
    2490.360,       // 125 C
    2526.929,       // 130 C
    2562.815,       // 135 C
    2598.018,       // 140 C
    2632.594,       // 145 C
    2666.540,       // 150 C
    2699.875,       // 155 C
    2732.614,       // 160 C
    2764.790,       // 165 C
    2796.384,       // 170 C
    2827.445,       // 175 C
    2857.952,       // 180 C
    2887.953,       // 185 C
    2917.445,       // 190 C
    2946.440,       // 195 C
    2974.950,       // 200 C
};


/** @brief Uses LUT_ADC to convert raw ADC value to Celsius. */
__attribute__((unused)) static float Look_Up_Table_Calc(uint16_t raw_value){

    float ADC_read = (float)raw_value;

    if(ADC_read <= LUT_ADC[0]){
        return LUT_START_TEMP;
    }else if(ADC_read >= LUT_ADC[LUT_SIZE - 1]){
        return LUT_END_TEMP;
    }

    // Binary Search to find the bounding indices
    int low = 0;
    int high = LUT_SIZE - 1;

    while (low < high - 1) {
        int mid = low + (high - low) / 2; 

        if (ADC_read >= LUT_ADC[mid]) {
            low = mid; 
        } else {
            high = mid; 
        }
    }

    // Extract the boundaries using our found indices
    float r_low = LUT_ADC[low];
    float r_high = LUT_ADC[high];
    
    float t_low = LUT_START_TEMP + (low * LUT_STEP);
    float t_high = t_low + LUT_STEP;

    // Linear Interpolation Formula
    float temperature = t_low + (ADC_read - r_low) * ((t_high - t_low) / (r_high - r_low));
    
    return temperature;
    
}

void vTempTask(void *pvParameters){
    (void)pvParameters;

    for (;;)
    {
        /* Suspend self until waken up / resumed */
        vTaskSuspend(NULL);

        // for(size_t i = 0; i < NUM_PT1000; i++)
        // {
            
        // }

    }

}
