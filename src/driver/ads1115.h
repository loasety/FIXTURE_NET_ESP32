#pragma once

#include "global.h"
#include <Arduino.h>

/**
 * @brief ADS1115 模数转换器驱动类
 * 
 * 功能:
 * 1. 初始化ADS1115外设和中断引脚(ADC_OK)
 * 2. 处理差分/单端通道的数据采集
 * 3. 换算电池电压、输出电压、输出电流
 */
class ADS1115_Driver {
public:
    /**
     * @brief 初始化ADC硬件和中断
     * 
     * @return true 初始化成功
     * @return false 初始化失败
     */
    static bool begin();

    /**
     * @brief 读取并换算所有通道数据
     * 
     * @param v_cso_iout 输出电流(对应通道0)
     * @param v_out 输出电压(对应通道1)
     * @param v_bat 电池电压(对应通道2)
     */
    static void readAll(float &v_cso_iout, float &v_out, float &v_bat, float &raw_cso);

private:
    static volatile bool dataReady;
    static void IRAM_ATTR isr();
    static bool startConversion(uint8_t channel);
    static int16_t readResult();
    static int16_t getSingle(uint8_t channel);
};
