#include "../include/global.h"
#include "../core/system_state.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static void taskRstLoop(void* pvParameters) {
    // 初始化引脚，默认拉低
    pinMode(RST_CTR_PIN, OUTPUT);
    digitalWrite(RST_CTR_PIN, LOW);
    
    // 只有当 NVS 里启用了该功能时才执行复位时序
    if (SystemState::config.EN_RST) {
        Serial.println("[RST_CTR]RST_OPEN...");
        
        // 1. 等待开机后的计时参数 (RSTON_TIME)
        if (SystemState::config.RSTON_TIME > 0) {
            vTaskDelay(pdMS_TO_TICKS(SystemState::config.RSTON_TIME));
        }
        
        // 2. 计时到后，拉高引脚
        digitalWrite(RST_CTR_PIN, HIGH);
        Serial.println("[RST_CTR]RST_LOW");

        // 3. 维持高电平 (RSTOFF_TIME)
        if (SystemState::config.RSTOFF_TIME > 0) {
            vTaskDelay(pdMS_TO_TICKS(SystemState::config.RSTOFF_TIME));
        }
        
        // 4. 时间到后，恢复默认的低电平
        digitalWrite(RST_CTR_PIN, LOW);
        Serial.println("[RST_CTR]RST_HIGH");

    } else {
        Serial.println("[RST_CTR]RST_DISABLED");
    }
    
    // 由于该复位行为开机只执行一次，执行完毕后直接销毁自身任务以节省内存
    vTaskDelete(NULL);
}

void start_task_rst() {
    xTaskCreatePinnedToCore(taskRstLoop, "Task_RST", 2048, NULL, 1, NULL, 1);
}
