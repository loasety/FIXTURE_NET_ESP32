#include "nvs_manager.h"
#include "../core/system_state.h"
#include <Preferences.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

volatile bool NVSManager::saveRequested = false;
uint32_t NVSManager::lastRequestTime = 0;
static Preferences prefs;

void NVSManager::init() {
    prefs.begin("sys_cfg", false); // 读写模式
    
    // 如果是第一次运行，NVS里没有，就写入当前 system_state 里的默认值
    if (!prefs.isKey("I_MAX")) {
        Serial.println("[NVS] First write param...");
        prefs.putFloat("I_MAX", SystemState::config.I_MAX);
        prefs.putFloat("I_MIN", SystemState::config.I_MIN);
        prefs.putFloat("I_Start", SystemState::config.I_Start);
        prefs.putFloat("SET_VOUT", SystemState::config.SET_VOUT);
        prefs.putFloat("V_OUTMAX", SystemState::config.V_OUTMAX);
        prefs.putFloat("I_Offset", SystemState::config.I_Offset);
        prefs.putUInt("DelayStart", SystemState::config.DelayStart_ms);
        prefs.putString("DEV_NAME", String(SystemState::config.deviceName));
        prefs.putUInt("RSTON_T", 5000); // 默认 5000ms
        prefs.putUInt("RSTOFF_T", 2000); // 默认 2000ms
        prefs.putBool("EN_RST", false);  // 默认关闭
    } else {
        Serial.println("[NVS] read Sys param...");
        SystemState::config.I_MAX = prefs.getFloat("I_MAX", SystemState::config.I_MAX);
        SystemState::config.I_MIN = prefs.getFloat("I_MIN", SystemState::config.I_MIN);
        SystemState::config.I_Start = prefs.isKey("I_Start") ? prefs.getFloat("I_Start") : SystemState::config.I_Start;
        SystemState::config.SET_VOUT = prefs.isKey("SET_VOUT") ? prefs.getFloat("SET_VOUT") : SystemState::config.SET_VOUT;
        SystemState::config.V_OUTMAX = prefs.isKey("V_OUTMAX") ? prefs.getFloat("V_OUTMAX") : SystemState::config.V_OUTMAX;
        SystemState::config.I_Offset = prefs.isKey("I_Offset") ? prefs.getFloat("I_Offset") : SystemState::config.I_Offset;
        SystemState::config.DelayStart_ms = prefs.isKey("DelayStart") ? prefs.getUInt("DelayStart") : SystemState::config.DelayStart_ms;
        
        String name = prefs.getString("DEV_NAME", String(SystemState::config.deviceName));
        strncpy(SystemState::config.deviceName, name.c_str(), sizeof(SystemState::config.deviceName) - 1);
        SystemState::config.deviceName[sizeof(SystemState::config.deviceName) - 1] = '\0';
        
        SystemState::config.RSTON_TIME = prefs.isKey("RSTON_T") ? prefs.getUInt("RSTON_T") : 5000;
        SystemState::config.RSTOFF_TIME = prefs.isKey("RSTOFF_T") ? prefs.getUInt("RSTOFF_T") : 2000;
        SystemState::config.EN_RST = prefs.isKey("EN_RST") ? prefs.getBool("EN_RST") : false;
        
        // 确保 V_OUTMAX 绑定
        SystemState::config.V_OUTMAX = SystemState::config.SET_VOUT + 0.4f;
    }
}

void NVSManager::requestSave() {
    saveRequested = true;
    lastRequestTime = millis();
}

void NVSManager::taskNvsLoop(void* pvParameters) {
    for(;;) {
        if (saveRequested) {
            // 如果连续3秒没有新的保存请求，说明参数修改完毕，真正执行写入
            if (millis() - lastRequestTime > 3000) {
                saveRequested = false;
                Serial.println("[NVS] Write param to Flash...");
                
                prefs.putFloat("I_MAX", SystemState::config.I_MAX);
                prefs.putFloat("I_MIN", SystemState::config.I_MIN);
                prefs.putFloat("I_Start", SystemState::config.I_Start);
                prefs.putFloat("SET_VOUT", SystemState::config.SET_VOUT);
                prefs.putFloat("V_OUTMAX", SystemState::config.V_OUTMAX);
                prefs.putFloat("I_Offset", SystemState::config.I_Offset);
                prefs.putUInt("DelayStart", SystemState::config.DelayStart_ms);
                prefs.putString("DEV_NAME", String(SystemState::config.deviceName));
                
                prefs.putUInt("RSTON_T", SystemState::config.RSTON_TIME);
                prefs.putUInt("RSTOFF_T", SystemState::config.RSTOFF_TIME);
                prefs.putBool("EN_RST", SystemState::config.EN_RST);
                
                Serial.println("[NVS] 保存完成。");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(500)); // 每半秒检查一次，极低开销
    }
}

void start_task_nvs() {
    xTaskCreatePinnedToCore(NVSManager::taskNvsLoop, "Task_NVS", 4096, NULL, 1, NULL, 1);
}
