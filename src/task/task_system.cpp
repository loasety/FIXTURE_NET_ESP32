#include "../include/global.h"
#include "../core/system_state.h"
#include "../driver/sc8726a.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define BUTTON_PIN 0 // 开发板 BOOT 按键

static void checkButton() {
    static uint32_t press_count = 0;
    static bool btn_handled = false;
    
    if (digitalRead(BUTTON_PIN) == LOW) {
        press_count++;
        // 任务循环是 100ms 一次，30 次正好是 3 秒
        if (press_count >= 30 && !btn_handled) {
            btn_handled = true; // 防误触，直到松开按键才允许下一次长按
            SystemState::Wireless_Enabled = !SystemState::Wireless_Enabled;
            
            if (SystemState::Wireless_Enabled) {
                SystemState::wireless_ActiveTime = millis(); // 记录开启时间
                Serial.println("\n[SYSTEM]OPEN　WIFI");
            } else {
                Serial.println("\n[SYSTEM]CLOSE WIFI");
            }
        }
    } else {
        press_count = 0;
        btn_handled = false;
    }
}

static void checkBattery(float vbat) {
    if (vbat < 3.2f) SystemState::bat_State = BAT_LOW;
    else if (vbat < 3.4f) SystemState::bat_State = BAT_25;
    else if (vbat < 3.65f) SystemState::bat_State = BAT_50;
    else if (vbat < 3.95f) SystemState::bat_State = BAT_75;
    else SystemState::bat_State = BAT_100;
}

static void checkCharge() {
    bool isChrg = (digitalRead(STATE_CHRG_PIN) == LOW);
    bool isFull = (digitalRead(STATE_OK_PIN) == LOW);
    if (isFull) SystemState::chg_State = CHRG_FULL;
    else if (isChrg) SystemState::chg_State = CHRG_ING;
    else SystemState::chg_State = CHRG_IDLE;
}

static void checkOutput(float i_out, float v_out) {
    if (SystemState::emergency_Stop) return; // 如果已经触发紧急断电，保持状态锁定
    
    // 极速过压保护
    if (v_out > SystemState::config.V_OUTMAX) {
        SC8726A_Driver::setCE(false);
        SystemState::Over_Voltage = true;
        SystemState::emergency_Stop = true;
        SystemState::out_State = OUT_ERROR_OVP;
        Serial.println("[ERROR] OVP OUT_OFF");
        return;
    }

    // 带延时的启动灯自锁逻辑：只有当 i_out > I_Start 持续满足 DelayStart_ms，才锁定 START 灯
    static uint32_t startTimer = 0;
    static bool isCounting = false;
    
    if (i_out > SystemState::config.I_Start) {
        if (!isCounting) {
            isCounting = true;
            startTimer = millis();
        } else {
            // 如果持续计时超过了设定的延时（如5秒），才触发自锁
            if (millis() - startTimer > SystemState::config.DelayStart_ms) {
                SystemState::Start_Delayed = true;
            }
        }
    } 

    // 主状态机：电流区间判定
    if (i_out > SystemState::config.I_MAX) {
        // 超过 IMAX，立即物理断电，并亮红灯
        SC8726A_Driver::setCE(false);
        SystemState::emergency_Stop = true;
        SystemState::out_State = OUT_OC; // 亮过流灯
        SystemState::Start_Delayed = false; // 清除其他灯
        isCounting = false; // 清空启动计时器
        Serial.println("[ERROR] OCP OUT_OFF");
    } 
    else if (i_out > SystemState::config.I_MIN) {
        // 大于 IMIN 且小于 IMAX，正常工作
        SystemState::out_State = OUT_OK; // 亮 LED_OK
    } 
    else {
        // 电流小于 IMIN，跌入欠流区
        SystemState::out_State = OUT_UC; // 灭别的灯，只亮欠流灯
        SystemState::Start_Delayed = false; // 跌破最低下限，解除 START 锁定
        isCounting = false; // 清空启动计时器
    }
}

static void taskSystemLoop(void* pvParameters) {
    pinMode(STATE_CHRG_PIN, INPUT_PULLUP);
    pinMode(STATE_OK_PIN, INPUT_PULLUP);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(100); 
    
    for(;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        checkButton(); // 检测 GPIO0 按钮
        // 从全局高速传感器缓存中读取数据，而不是再次去阻塞 I2C 硬件
        checkBattery(SystemState::sensor.battery_voltage);
        checkCharge();
        checkOutput(SystemState::sensor.output_current, SystemState::sensor.output_voltage);
    }
}

void start_task_system() {
    xTaskCreatePinnedToCore(taskSystemLoop, "Task_System", 4096, NULL, 5, NULL, 1);
}
