#pragma once

#include "global.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

struct SensorData {
    float battery_voltage;
    float output_voltage;
    float output_current;
    float raw_cso_voltage; // 供校准使用的原始传感器电压
    uint32_t timestamp;
    bool adc_ok;
};

struct SystemConfig {
    float I_MAX;
    float I_MIN;
    float I_Start;
    float SET_VOUT;
    float V_OUTMAX;
    float I_Offset; // SC8726A CSO 零点本底电压
    uint32_t DelayStart_ms;
    char deviceName[32]; // 治具设备名称，如 FIXTURE_01
    
    // 外部设备复位控制参数
    uint32_t RSTON_TIME;   // 开机后延时多久拉高
    uint32_t RSTOFF_TIME;  // 拉高后维持多久拉低
    bool     EN_RST;       // 是否启用该复位功能
};

class SystemState {
public:
    static volatile BatteryState bat_State;
    static volatile ChargeState  chg_State;
    static volatile OutputState  out_State;
    
    // 极速保护信号
    static volatile bool Over_Voltage;
    static volatile bool emergency_Stop;
    
    // 独立启动状态指示（高于 I_Start 并满 5 秒）
    static volatile bool Start_Delayed;
    
    // OTA 升级模式标志位
    static volatile bool Ota_Mode;
    
    // 无线射频按需唤醒状态机
    static volatile bool Wireless_Enabled;
    static volatile uint32_t wireless_ActiveTime;
    
    // 全局数据中枢
    static SensorData sensor;
    static SystemConfig config;
    
    // 任务句柄，用于高速通知
    static TaskHandle_t taskPowerHandle;
};
