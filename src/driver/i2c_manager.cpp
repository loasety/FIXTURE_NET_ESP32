#include "i2c_manager.h"
#include <Wire.h>
#include <Arduino.h>

SemaphoreHandle_t I2CManager::mutex = NULL;
bool I2CManager::initialized = false;

bool I2CManager::begin() {
    if (initialized) return true;
    
    bool ret = Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    if (ret) {
        mutex = xSemaphoreCreateMutex();
        if (mutex != NULL) {
            initialized = true;
            return true;
        }
    }
    return false;
}

bool I2CManager::lock(uint32_t timeout_ms) {
    if (!initialized || !mutex) return false;
    return (xSemaphoreTake(mutex, pdMS_TO_TICKS(timeout_ms)) == pdTRUE);
}

void I2CManager::unlock() {
    if (initialized && mutex) {
        xSemaphoreGive(mutex);
    }
}

bool I2CManager::recover() {
    // I2C 恢复序列: 生成9个SCL脉冲释放SDA
    pinMode(I2C_SDA_PIN, INPUT_PULLUP);
    pinMode(I2C_SCL_PIN, OUTPUT);
    
    for (int i = 0; i < 9; i++) {
        digitalWrite(I2C_SCL_PIN, LOW);
        delayMicroseconds(5);
        digitalWrite(I2C_SCL_PIN, HIGH);
        delayMicroseconds(5);
        if (digitalRead(I2C_SDA_PIN) == HIGH) {
            break; // SDA已释放
        }
    }
    
    // 生成 STOP 信号
    pinMode(I2C_SDA_PIN, OUTPUT);
    digitalWrite(I2C_SDA_PIN, LOW);
    delayMicroseconds(5);
    digitalWrite(I2C_SCL_PIN, HIGH);
    delayMicroseconds(5);
    digitalWrite(I2C_SDA_PIN, HIGH);
    delayMicroseconds(5);
    
    initialized = false;
    if (mutex) {
        vSemaphoreDelete(mutex);
        mutex = NULL;
    }
    return begin();
}

uint8_t I2CManager::scan() {
    if (!lock(100)) return 0;
    
    uint8_t count = 0;
    for (uint8_t address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        uint8_t error = Wire.endTransmission();
        if (error == 0) {
            count++;
        }
    }
    
    unlock();
    return count;
}
