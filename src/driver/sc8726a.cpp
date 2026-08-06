#include "sc8726a.h"
#include "i2c_manager.h"
#include <Arduino.h>
#include <Wire.h>

bool SC8726A_Driver::begin() {
    pinMode(SC8726A_CE_PIN, OUTPUT);
    digitalWrite(SC8726A_CE_PIN, HIGH); // 默认打开输出
    return true;
}

void SC8726A_Driver::setCE(bool state) {
    digitalWrite(SC8726A_CE_PIN, state ? HIGH : LOW);
}

bool SC8726A_Driver::setVoltage(float targetV) {

    if (targetV < 2.4f) targetV = 2.4f;
    if (targetV > 22.0f) targetV = 22.0f;
    
    float offsetV = 0.0f;
    bool is_decrement = false;
    
    if (targetV >= 5.0f) {
        offsetV = targetV - 5.0f;
        is_decrement = false;
    } else {
        offsetV = 5.0f - targetV;
        is_decrement = true;
    }
    
    uint16_t offset_steps = (uint16_t)((offsetV * 1000.0f) / 20.0f); 
    
    uint8_t reg03_val = (offset_steps >> 2) & 0xFF;
    uint8_t reg04_val = offset_steps & 0x03;
    reg04_val |= 0x18; 
    
    // 如果是下行调压(减量)，置位 Bit2(FB_DIR=1)
    if (is_decrement) {
        reg04_val |= 0x04;
    }    
    if (I2CManager::lock(100)) {
        Wire.beginTransmission(SC8726A_ADDR);
        Wire.write(0x03); Wire.write(reg03_val);
        Wire.endTransmission();
        
        Wire.beginTransmission(SC8726A_ADDR);
        Wire.write(0x04); Wire.write(reg04_val);
        Wire.endTransmission();
        
        // 激活电压更新
        Wire.beginTransmission(SC8726A_ADDR);
        Wire.write(0x05); Wire.write(0x02); 
        Wire.endTransmission();
        
        I2CManager::unlock();
        return true;
    }
    return false; // 超时未拿到锁
}
