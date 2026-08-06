#include "../include/global.h"
#include "../core/system_state.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static void taskLedLoop(void* pvParameters) {
    pinMode(LED_25_PIN, OUTPUT); pinMode(LED_50_PIN, OUTPUT);
    pinMode(LED_75_PIN, OUTPUT); pinMode(LED_100_PIN, OUTPUT);
    pinMode(LED_UC_PIN, OUTPUT); pinMode(LED_OC_PIN, OUTPUT);
    pinMode(LED_OK_PIN, OUTPUT); pinMode(LED_START_PIN, OUTPUT);
    
    bool blinkState = false;
    for(;;) {
        blinkState = !blinkState;
        
        digitalWrite(LED_25_PIN, HIGH); digitalWrite(LED_50_PIN, HIGH);
        digitalWrite(LED_75_PIN, HIGH); digitalWrite(LED_100_PIN, HIGH);
        
        if (SystemState::chg_State == CHRG_FULL) {
            digitalWrite(LED_25_PIN, LOW); digitalWrite(LED_50_PIN, LOW);
            digitalWrite(LED_75_PIN, LOW); digitalWrite(LED_100_PIN, LOW);
        } else if (SystemState::chg_State == CHRG_ING) {            
            // 正在充电且没有发生故障时，跑马灯动画
            if (SystemState::bat_State == BAT_LOW) {
                digitalWrite(LED_25_PIN, blinkState ? LOW : HIGH);
            } else if (SystemState::bat_State == BAT_25) {
                digitalWrite(LED_25_PIN, LOW);
                digitalWrite(LED_50_PIN, blinkState ? LOW : HIGH);
            } else if (SystemState::bat_State == BAT_50) {
                digitalWrite(LED_25_PIN, LOW); digitalWrite(LED_50_PIN, LOW);
                digitalWrite(LED_75_PIN, blinkState ? LOW : HIGH);
            } else if (SystemState::bat_State == BAT_75 || SystemState::bat_State == BAT_100) {
                // 只要还没收到硬件的 CHRG_FULL 信号，哪怕电压已经达到了 BAT_100，第四个灯也要一直闪！
                digitalWrite(LED_25_PIN, LOW); digitalWrite(LED_50_PIN, LOW);
                digitalWrite(LED_75_PIN, LOW);
                digitalWrite(LED_100_PIN, blinkState ? LOW : HIGH);
            }
        } else {
            // 放电状态，或者发生过流/过压紧急故障时，强制常亮显示当前实际电量，停止闪烁动画
            if (SystemState::bat_State == BAT_LOW) {
                digitalWrite(LED_25_PIN, blinkState ? LOW : HIGH);
            } else if (SystemState::bat_State != BAT_EMPTY) {
                if (SystemState::bat_State >= BAT_25) digitalWrite(LED_25_PIN, LOW);
                if (SystemState::bat_State >= BAT_50) digitalWrite(LED_50_PIN, LOW);
                if (SystemState::bat_State >= BAT_75) digitalWrite(LED_75_PIN, LOW);
                if (SystemState::bat_State >= BAT_100) digitalWrite(LED_100_PIN, LOW);
            }
        }
        
        // 默认关闭所有输出状态灯 (HIGH 为灭)
        digitalWrite(LED_UC_PIN, HIGH); digitalWrite(LED_OC_PIN, HIGH);
        digitalWrite(LED_OK_PIN, HIGH); digitalWrite(LED_START_PIN, HIGH);
        
        if (SystemState::Wireless_Enabled) {
            // OTA 模式下，接管输出指示灯，显示 WiFi 启动的递进加载动画 (GPIO5 -> 4 -> 3 -> 2)
            static uint8_t wifiLedStep = 0;
            if (wifiLedStep >= 1) digitalWrite(LED_UC_PIN, LOW); // GPIO5
            if (wifiLedStep >= 2) digitalWrite(LED_OC_PIN, LOW); // GPIO4
            if (wifiLedStep >= 3) digitalWrite(LED_OK_PIN, LOW); // GPIO3
            if (wifiLedStep >= 4) digitalWrite(LED_START_PIN, LOW); // GPIO2
            
            wifiLedStep++;
            if (wifiLedStep > 4) wifiLedStep = 0; // 从全灭开始下一轮循环
        } else if (SystemState::Over_Voltage) {
            digitalWrite(LED_UC_PIN, blinkState ? LOW : HIGH); digitalWrite(LED_OC_PIN, blinkState ? LOW : HIGH);
            digitalWrite(LED_OK_PIN, blinkState ? LOW : HIGH); digitalWrite(LED_START_PIN, blinkState ? LOW : HIGH);
        } else {
            switch(SystemState::out_State) {
                case OUT_OK:    digitalWrite(LED_OK_PIN, LOW); break;
                case OUT_UC:    digitalWrite(LED_UC_PIN, LOW); break;
                case OUT_OC:    digitalWrite(LED_OC_PIN, LOW); break;
                default: break;
            }
            // 独立的 START 指示逻辑（可与 OK 同亮）
            if (SystemState::Start_Delayed) {
                digitalWrite(LED_START_PIN, LOW); // 满 5 秒，点亮！
            }
        }
        vTaskDelay(pdMS_TO_TICKS(500)); 
    }
}

void start_task_led() {
    xTaskCreatePinnedToCore(taskLedLoop, "Task_LED", 2048, NULL, 2, NULL, 1);
}
