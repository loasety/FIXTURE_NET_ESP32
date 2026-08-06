#include "../include/global.h"
#include "../core/system_state.h"
#include "../driver/sc8726a.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static void taskPowerLoop(void* pvParameters) {
    for(;;) {
        // 极速阻塞等待来自高速ADC或其他异常源的切断通知
        // 此处不需要轮询，充分释放 CPU，直到收到信号
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        
        if (SystemState::emergency_Stop) {
            // 最快速度切断输出，绝不拖泥带水
            SC8726A_Driver::setCE(false);
            SystemState::out_State = OUT_ERROR_OVP; // 或者 OC，由发送方决定
            Serial.println("\n[POWER] OVP/OCP OUT_OFF");
        } else {
            // 如果未来有 ENABLE 请求，可以扩展在这里处理
            // SC8726A_Driver::setCE(true);
        }
    }
}

void start_task_power() {
    xTaskCreatePinnedToCore(
        taskPowerLoop, 
        "Task_Power", 
        2048, 
        NULL, 
        24, // 全局最高优先级
        &SystemState::taskPowerHandle, 
        1
    );
}
