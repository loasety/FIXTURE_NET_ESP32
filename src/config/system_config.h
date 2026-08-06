#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

#define ADS1115_ADDR 0x48
#define SC8726A_ADDR 0x62

extern float I_MAX;     // 最大充电电流
extern float I_MIN;     // 最小充电电流
extern float I_Start;   // 充电启动电流阈值
extern float SET_VOUT;  // 设定输出电压
extern float V_OUTMAX;  // 最大输出电压

// 电池电量状态，没电、低电、25%、50%、75%、100%
enum BatteryState { BAT_EMPTY, BAT_LOW, BAT_25, BAT_50, BAT_75, BAT_100 };
// 充电过程状态，空闲、充电中、充满
enum ChargeState  { CHRG_IDLE, CHRG_ING, CHRG_FULL };
// 输出状态机，输出关闭、欠压、正常、过流、启动中、错误
enum OutputState  { OUT_OFF, OUT_UC, OUT_OK, OUT_OC, OUT_START, OUT_ERROR_OVP };

#endif
