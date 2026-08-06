#include <Arduino.h>
#include "../include/global.h"
#include "../core/system_state.h"
#include "../driver/ads1115.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static void taskAdcLoop(void* pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    // IIR 低通滤波缓存
    float filtered_cso = 0.0f;
    float filtered_out = 0.0f;
    float filtered_bat = 0.0f;
    bool first_read = true;

    for(;;) {
        // 【功耗优化】动态采样率：
        // 如果系统输出关闭，或者当前电流处于欠流(无负载，< I_MIN)状态，
        // 则放宽到 200ms 采样，腾出时间供操作系统自动休眠。
        // 一旦接上负载 (> I_MIN)，切回 15ms 极速采样。
        TickType_t delayTicks = pdMS_TO_TICKS(15);
        bool should_be_low_power = (SystemState::out_State == OUT_OFF || (!first_read && filtered_cso < SystemState::config.I_MIN));
        
        if (should_be_low_power) {
            delayTicks = pdMS_TO_TICKS(200); 
        }
        
        vTaskDelayUntil(&xLastWakeTime, delayTicks);
        
        if (SystemState::emergency_Stop) {
            continue; // 如果已经断电保护了，暂时不再去抢占总线报警
        }

        float v_cso_iout = 0, v_out = 0, v_bat = 0, raw_cso = 0;
        ADS1115_Driver::readAll(v_cso_iout, v_out, v_bat, raw_cso);
        
        // 我们将原始电压记录在全局缓存中，以供外部(Task_Monitor)校准使用
        // 建议在 sensorData 中加入 raw_cso_voltage (目前暂未使用，如果想存也可以)
        
        // 如果读数异常极低(如开机初始化未完成)，强制抛弃，防止污染滤波器
        if (v_bat < 0.1f) {
            continue;
        }
        
        // 简易一阶IIR滤波，防止毛刺导致误触发保护 (电池电压变化慢，直接使用瞬态值避免开机阶梯爬升)
        if (first_read) {
            filtered_cso = v_cso_iout;
            filtered_out = v_out;
            filtered_bat = v_bat;
            first_read = false;
        } else {
            filtered_cso = filtered_cso * 0.7f + v_cso_iout * 0.3f;
            filtered_out = filtered_out * 0.7f + v_out * 0.3f;
            filtered_bat = v_bat; // 不做滤波，直接赋值
        }
        
        // 更新到全局数据中枢
        SystemState::sensor.output_current = filtered_cso;
        SystemState::sensor.output_voltage = filtered_out;
        SystemState::sensor.battery_voltage = filtered_bat;
        SystemState::sensor.raw_cso_voltage = raw_cso; // 传递原始本底给校准模块
        SystemState::sensor.timestamp = millis();

        // 极速过流/过压判定逻辑
        bool over_voltage = (filtered_out > SystemState::config.V_OUTMAX);
        bool over_current = (filtered_cso > SystemState::config.I_MAX);
        
        if (over_voltage || over_current) {
            SystemState::Over_Voltage = true;
            SystemState::emergency_Stop = true;
            SystemState::out_State = over_voltage ? OUT_ERROR_OVP : OUT_OC;
            
            // 越级通知 Task_Power 马上关断
            if (SystemState::taskPowerHandle != NULL) {
                xTaskNotifyGive(SystemState::taskPowerHandle);
            }
            Serial.printf("\n[ERROR]OUT ERROR:V_out=%.2f, I_out=%.2f\n", filtered_out, filtered_cso);
        }
    }
}

void start_task_adc() {
    xTaskCreatePinnedToCore(taskAdcLoop, "Task_ADC", 4096, NULL, 20, NULL, 1); // 极高优先级
}
