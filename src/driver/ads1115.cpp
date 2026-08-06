#include "ads1115.h"
#include "i2c_manager.h"
#include <Wire.h>
#include "../core/system_state.h"

volatile bool ADS1115_Driver::dataReady = false;

void IRAM_ATTR ADS1115_Driver::isr() {
    dataReady = true;
}

bool ADS1115_Driver::begin() {
    pinMode(ADC_OK_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(ADC_OK_PIN), isr, FALLING);
    
    // 可以在这里加个简单的通信测试，比如读一下 Config 寄存器
    if (I2CManager::lock(100)) {
        Wire.beginTransmission(ADS1115_ADDR);
        Wire.write(0x01); 
        uint8_t error = Wire.endTransmission();
        I2CManager::unlock();
        if (error != 0) return false;
        return true;
    }
    return false;
}

bool ADS1115_Driver::startConversion(uint8_t channel) {
    if (!I2CManager::lock(100)) return false;
    
    // 写入阈值寄存器，配置 ALERT/RDY 引脚，用于转换完成中断
    Wire.beginTransmission(ADS1115_ADDR);
    Wire.write(0x03); Wire.write(0x80); Wire.write(0x00);
    Wire.endTransmission();

    Wire.beginTransmission(ADS1115_ADDR);
    Wire.write(0x02); Wire.write(0x00); Wire.write(0x00);
    Wire.endTransmission();

    // 配置寄存器
    uint16_t config = 0x8000; 
    if (channel == 0)      { config |= (0x04 << 12); config |= (0x02 << 9); } 
    else if (channel == 1) { config |= (0x05 << 12); config |= (0x01 << 9); } 
    else if (channel == 2) { config |= (0x06 << 12); config |= (0x01 << 9); } 

    config |= (0x01 << 8); // 单次转换
    config |= (0x04 << 5); // 128SPS
    config |= 0x00;        // 传统比较器模式

    Wire.beginTransmission(ADS1115_ADDR);
    Wire.write(0x01); 
    Wire.write(config >> 8);
    Wire.write(config & 0xFF);
    Wire.endTransmission();
    
    I2CManager::unlock();
    return true;
}

int16_t ADS1115_Driver::readResult() {
    int16_t res = 0;
    if (I2CManager::lock(50)) {
        Wire.beginTransmission(ADS1115_ADDR);
        Wire.write(0x00); 
        Wire.endTransmission();
        
        Wire.requestFrom((uint16_t)ADS1115_ADDR, (uint8_t)2);
        if (Wire.available() >= 2) {
            res = (Wire.read() << 8) | Wire.read();
        }
        I2CManager::unlock();
    }
    return res;
}

int16_t ADS1115_Driver::getSingle(uint8_t channel) {
    dataReady = false;
    if (!startConversion(channel)) return 0;
    
    // 极速等待完成 (最高15ms，正常8ms)
    uint32_t waitTimeout = millis();
    while (!dataReady) {
        if (millis() - waitTimeout > 15) break; 
        vTaskDelay(pdMS_TO_TICKS(1)); 
    }
    
    if (dataReady) {
        return readResult();
    }
    return 0;
}

void ADS1115_Driver::readAll(float &v_cso_iout, float &v_out, float &v_bat, float &raw_cso) {
    // 高速单次读取
    int16_t raw0 = getSingle(0);
    int16_t raw1 = getSingle(1);
    int16_t raw2 = getSingle(2);
    
    // FSR 量程计算
    float adc0 = raw0 * (2.048f / 32768.0f);
    float adc1 = raw1 * (4.096f / 32768.0f);
    float adc2 = raw2 * (4.096f / 32768.0f);
    
    raw_cso = adc0; // 将原始 CSO 电压导出以供校准使用
    
    // 物理量换算 (利用系统动态偏移量 I_Offset)
    // 根据数据手册: IOUT = Vcso * 2kΩ / (Rs * Rcso)
    // 假设 Rs=10mΩ, Rcso=64kΩ, 则系数为 2000 / (0.01 * 64000) = 3.125
    float current_inst = (adc0 - SystemState::config.I_Offset) * 3.125f; 
    if (current_inst < 0.005f) current_inst = 0.0f; // 低于 5mA 视为 0
    v_cso_iout = current_inst;
    
    v_out = adc1 * (330.0f + 68.0f) / 68.0f;
    v_bat = adc2 * (10.0f + 10.0f) / 10.0f;
}
