#ifndef GLOBAL_DEF_H
#define GLOBAL_DEF_H

#include <Arduino.h>

// ==========================================
// 1. 固件版本信息 (降级保护与 OTA 鉴权使用)
// ==========================================
#define FIRMWARE_VERSION_MAJOR 1
#define FIRMWARE_VERSION_MINOR 0
#define FIRMWARE_VERSION_PATCH 0
#define FIRMWARE_VERSION_STR "1.0.0"

// NVS 数据库结构版本 (用于结构体升级迁移)
#define NVS_SCHEMA_VERSION 1

// ==========================================
// 2. 硬件引脚定义 (严格避开 OPI 冲突引脚)
// ==========================================
// I2C 总线
#define I2C_SCL_PIN 34
#define I2C_SDA_PIN 33

// ADS1115 (ADC)
#define ADC_OK_PIN  21  // 转换完成中断 (默认上拉)

// SC8726A (充放电控制器)
#define SC8726A_CE_PIN 17 // 高电平打开输出，低电平关闭

// 外围输出控制 (OUT_CRT已废弃，无需分配引脚)

// 预留测试控制针脚
#define CUSTOM_GPIO14_PIN 14 

#define LED_25_PIN  35
#define LED_50_PIN  36
#define LED_75_PIN  37
#define LED_100_PIN 38

// 系统状态指示 LED (高电平点亮)
#define LED_START_PIN 2 // 启动状态
#define LED_OK_PIN    3 // 正常状态
#define LED_OC_PIN    4 // 过流状态
#define LED_UC_PIN    5 // 欠流状态

// SLM6305 充电芯片状态反馈
#define STATE_CHRG_PIN 12 // 低电平=充电中
#define STATE_OK_PIN   13 // 低电平=已充满

// ==========================================
// 3. I2C 设备地址
// ==========================================
#define ADS1115_ADDR 0x48
#define SC8726A_ADDR 0x62

// ==========================================
// 4. 全局系统枚举与结构体
// ==========================================

// 系统核心状态机事件 (跨任务通信用)
enum SystemEvent {
    EVENT_NONE = 0,
    EVENT_POST_START,
    EVENT_POST_FAIL,
    EVENT_POST_PASS,
    EVENT_ADC_FAIL,
    EVENT_I2C_FAIL,
    EVENT_LOW_BAT,
    EVENT_OVP,         // 过压保护
    EVENT_OCP,         // 过流保护
    EVENT_UPDATE_START,
    EVENT_UPDATE_DONE,
    EVENT_ENTER_FACTORY,
    EVENT_ENTER_CALIBRATION
};

// 输出状态枚举 (由 Task_SC8726 统一管理 CE 引脚与输出)
enum OutputState {
    OUTPUT_DISABLE,
    OUTPUT_ENABLE,
    OUTPUT_OVP,
    OUTPUT_OCP,
    OUTPUT_ERROR
};

// 系统配置参数 (保存在 NVS 中)
struct SystemConfig {
    uint8_t schema_version;
    char device_id[16];        // 例如 "JIG001"
    
    // 校准参数
    float current_gain;
    float current_offset;
    float voltage_gain;
    float voltage_offset;
    
    // 工作参数限制
    float max_vout;
    float min_vout;
    float current_limit;
    
    // WiFi/OTA
    char wifi_ssid[32];
    char wifi_pass[64];
    char ota_token[64];
};

// 预定义的一些安全边界限制
#define VOUT_MAX_HARD_LIMIT 22.0f  // 绝对最大输出电压 22V
#define OVER_VOLTAGE_MARGIN 0.4f   // V_OUTMAX = SET_VOUT + 400mV

#endif // GLOBAL_DEF_H
