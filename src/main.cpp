#include <Arduino.h>
#include "include/global.h"
#include "storage/nvs_manager.h"
#include "driver/i2c_manager.h"
#include "driver/sc8726a.h"
#include "driver/ads1115.h"
#include "core/system_state.h"
#include <WiFi.h>

extern void start_task_nvs();
extern void start_task_power();
extern void start_task_adc();
extern void start_task_system();
extern void start_task_led();
extern void start_task_monitor();
extern void start_task_wireless();
extern void start_task_rst();

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    // 应要求，将全局 CPU 频率锁定在 80MHz 以优化整体功耗
    setCpuFrequencyMhz(160);
    
    Serial.println("\n[SYSTEM] Init...");
    
    // 1. 读取持久化参数
    NVSManager::init();
    
    // 2. 初始化驱动层
    if (!I2CManager::begin()) { Serial.println("[ERROR] Init I2C"); }
    if (!SC8726A_Driver::begin()) { Serial.println("[WARN] SC8726A Init"); }
    if (!ADS1115_Driver::begin()) { Serial.println("[WARN] ADS1115 Init"); }
    
    // 3. 根据读取到的参数设定硬件
    SC8726A_Driver::setVoltage(SystemState::config.SET_VOUT);
    SC8726A_Driver::setCE(true); // 默认开启，若过压过流会被极速切断
    
    // 关闭WiFi以节省功耗，并确保蓝牙未被初始化
    WiFi.mode(WIFI_OFF);
    
    // 4. 初始化所有任务
    start_task_nvs();
    start_task_power();    // 优先级 24 (最高执行)
    start_task_adc();      // 优先级 20 (极速采样)
    start_task_system();   // 优先级 5
    start_task_led();      // 优先级 2
    start_task_monitor();  // 优先级 2
    start_task_wireless(); // WiFi OTA后台服务 (按需初始化，默认休眠等待)
    start_task_rst();      // 外部设备定时复位控制 (仅开机执行一次即销毁)
    
    Serial.println("[SYSTEM] Setup completed. Entering OS schedule...");
    vTaskDelete(NULL);
}

void loop() {}
