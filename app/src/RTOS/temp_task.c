#include <stdint.h>


#define LUT_START_TEMP  -30
#define LUT_END_TEMP    200
#define LUT_STEP        5
#define LUT_SIZE ((LUT_END_TEMP - LUT_START_TEMP) / LUT_STEP + 1)
const float LUT_R[LUT_SIZE] = {
    882.22f,    // -30
    901.92f,    // -25
    921.6f,     // -20
    941.24f,    // -15
    960.86f,    // -10
    980.44f,    // -5
    1000.0f,    // 0
    1019.53f,   // 5
    1039.03f,   // 10
    1058.49f,   // 15
    1077.94f,   // 20
    1097.35f,   // 25
    1116.73f,   // 30
    1136.08f,   // 35
    1155.41f,   // 40
    1174.7f,    // 45
    1193.97f,   // 50
    1213.21f,   // 55
    1232.42f,   // 60
    1251.6f,    // 65
    1270.75f,   // 70
    1289.87f,   // 75
    1308.97f,   // 80 
    1328.03f,   // 85
    1347.07f,   // 90
    1366.08f,   // 95
    1385.05f,   // 100
    1404.0f,    // 105
    1422.93f,   // 110
    1441.82f,   // 115
    1460.68f,   // 120
    1479.51f,   // 125
    1498.32f,   // 130
    1517.1f,    // 135
    1535.84f,   // 140
    1554.56f,   // 145
    1573.25f,   // 150
    1591.91f,   // 155
    1610.54f,   // 160
    1629.15f,   // 165
    1647.72f,   // 170
    1666.27f,   // 175
    1684.78f,   // 180
    1703.27f,   // 185
    1721.73f,   // 190
    1740.16f,   // 195
    1758.56f    // 200
};

float LUT_ADC[LUT_SIZE];

/** @brief Calculates raw ADC values from LUT_R. */
static void init_PT1000_LUT() {
    for (int i = 0; i < LUT_SIZE; i++) {
        float R = LUT_R[i];
        
        float K = R / 680.0f;
        
        // Calculate expected voltage ratio (0.0 to 1.0)
        float V = 1.65f * ((K - 1.0f) / (K + 1.0f));
        
        LUT_ADC[i] = V * 4096.0f;
    }
}

/** @brief Uses LUT_ADC to convert raw ADC value to Celsius. */
static float Look_Up_Table_Calc(uint32_t raw_value) {

    if (raw_value <= LUT_ADC[0]) {
        return LUT_START_TEMP;
    } else if (raw_value >= LUT_ADC[LUT_SIZE - 1]) {
        return LUT_END_TEMP;
    }

    // Binary Search to find the bounding indices
    int low = 0;
    int high = LUT_SIZE - 1;

    while (low < high - 1) {
        int mid = low + (high - low) / 2; 

        if (raw_value >= LUT_ADC[mid]) {
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
    float temperature = t_low + (raw_value - r_low) * ((t_high - t_low) / (r_high - r_low));
    
    return temperature;
    
}

void vTempTask(void *pvParameters){

    init_PT1000_LUT();

}
