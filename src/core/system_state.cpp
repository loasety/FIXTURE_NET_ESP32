#include "system_state.h"

volatile BatteryState SystemState::bat_State = BAT_EMPTY;
volatile ChargeState  SystemState::chg_State = CHRG_IDLE;
volatile OutputState  SystemState::out_State = OUT_OFF;
volatile bool         SystemState::Over_Voltage = false;//过压状态
volatile bool         SystemState::emergency_Stop = false;//紧急事件
volatile bool         SystemState::Start_Delayed = false;//启动延时状态（高于 I_Start 并满 5 秒）
volatile bool         SystemState::Ota_Mode = false;//OTA 升级模式标志位
volatile bool         SystemState::Wireless_Enabled = false;//无线射频按需唤醒状态机
volatile uint32_t     SystemState::wireless_ActiveTime = 0;//无线射频按需唤醒状态机

TaskHandle_t SystemState::taskPowerHandle = NULL;

// 传感器数据缓存初始化
SensorData SystemState::sensor = {0.0f, 0.0f, 0.0f, 0.0f, 0, false};

// 配置参数的内存映射 (启动时将由 NVS 覆盖，这里只是兜底默认值)
SystemConfig SystemState::config = {
    1.0f,    // I_MAX
    0.1f,    // I_MIN
    0.2f,    // I_Start
    5.2f,   // SET_VOUT
    5.6f,   // V_OUTMAX (始终 = SET_VOUT + 0.4)
    0.0101f, // I_Offset
    5000,    // DelayStart_ms
    "FIXTURE_01", // deviceName
    5000,    // RSTON_TIME
    2000,    // RSTOFF_TIME
    false    // EN_RST
};
