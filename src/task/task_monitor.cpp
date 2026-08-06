#include "../include/global.h"
#include "../core/system_state.h"
#include "../storage/nvs_manager.h"
#include "../driver/sc8726a.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static void handleSerialCommand(String cmd) {
    cmd.trim();
    if (cmd.length() == 0) return;
    
    int spaceIdx = cmd.indexOf(' ');
    String key, valStr;
    if (spaceIdx != -1) {
        key = cmd.substring(0, spaceIdx);
        valStr = cmd.substring(spaceIdx + 1);
    } else {
        key = cmd;
        valStr = "";
    }
    key.toUpperCase();
    
    bool changed = false;
    
    if (key == "VOUT") {
        float val = valStr.toFloat();
        if (val >= 2.4f && val <= 22.0f) {
            SystemState::config.SET_VOUT = val;
            SystemState::config.V_OUTMAX = val + 0.4f; // 联动
            SC8726A_Driver::setVoltage(val);
            changed = true;
            Serial.printf("[CMD] VOUT 设为 %.2f V, V_OUTMAX 联动为 %.2f V\n", val, SystemState::config.V_OUTMAX);
        }
    } 
    else if (key == "IMAX") {
        SystemState::config.I_MAX = valStr.toFloat();
        changed = true;
        Serial.printf("[CMD] I_MAX 设为 %.2f A\n", SystemState::config.I_MAX);
    }
    else if (key == "IMIN") {
        SystemState::config.I_MIN = valStr.toFloat();
        changed = true;
        Serial.printf("[CMD] I_MIN 设为 %.2f A\n", SystemState::config.I_MIN);
    }
    else if (key == "ISTART") {
        SystemState::config.I_Start = valStr.toFloat();
        changed = true;
        Serial.printf("[CMD] I_Start 设为 %.2f A\n", SystemState::config.I_Start);
    }
    else if (key == "DELAY") {
        SystemState::config.DelayStart_ms = (uint32_t)valStr.toInt();
        changed = true;
        Serial.printf("[CMD] DelayStart 设为 %d ms\n", SystemState::config.DelayStart_ms);
    }
    else if (key == "CALIB_I" || cmd == "CALIB_I") { // 支持带空格或不带参数
        // 使用当前的原始传感器本底电压作为新的零点偏置
        float current_raw = SystemState::sensor.raw_cso_voltage;
        SystemState::config.I_Offset = current_raw;
        changed = true;
        Serial.printf("[CMD] CALIB_I 成功! SC8726A CSO 电流零点已校准为本底电压: %.4f V\n", current_raw);
    }
    else if (key == "NAME") {
        if (valStr.length() > 0 && valStr.length() < 32) {
            strncpy(SystemState::config.deviceName, valStr.c_str(), sizeof(SystemState::config.deviceName) - 1);
            SystemState::config.deviceName[sizeof(SystemState::config.deviceName) - 1] = '\0';
            changed = true;
            Serial.printf("[CMD] 设备名变更为: %s\n", SystemState::config.deviceName);
            // 提示：这会同步改变 WiFi OTA 升级时的热点名称。下次开启 WiFi 升级模式时生效。
        }
    }
    else if (key == "RSTON_TIME") {
        SystemState::config.RSTON_TIME = (uint32_t)valStr.toInt();
        changed = true;
        Serial.printf("[CMD] RSTON_TIME 设为 %d ms\n", SystemState::config.RSTON_TIME);
    }
    else if (key == "RSTOFF_TIME") {
        SystemState::config.RSTOFF_TIME = (uint32_t)valStr.toInt();
        changed = true;
        Serial.printf("[CMD] RSTOFF_TIME 设为 %d ms\n", SystemState::config.RSTOFF_TIME);
    }
    else if (key == "OPEN_RST" || cmd == "OPEN_RST") {
        SystemState::config.EN_RST = true;
        changed = true;
        Serial.println("[CMD] 外部设备复位控制 (RST_CTR) 已开启，下次启动生效");
    }
    else if (key == "CLOSE_RST" || cmd == "CLOSE_RST") {
        SystemState::config.EN_RST = false;
        changed = true;
        Serial.println("[CMD] 外部设备复位控制 (RST_CTR) 已关闭");
    }
    else if (key == "GET_CFG" || cmd == "GET_CFG") {
        Serial.printf("[CFG] IMAX=%.2f, IMIN=%.2f, ISTART=%.2f, VOUT=%.2f, DELAY=%d, IOFFSET=%.4f, NAME=%s, RSTON=%d, RSTOFF=%d, EN_RST=%d\n",
            SystemState::config.I_MAX,
            SystemState::config.I_MIN,
            SystemState::config.I_Start,
            SystemState::config.SET_VOUT,
            SystemState::config.DelayStart_ms,
            SystemState::config.I_Offset,
            SystemState::config.deviceName,
            SystemState::config.RSTON_TIME,
            SystemState::config.RSTOFF_TIME,
            SystemState::config.EN_RST ? 1 : 0
        );
    }
    else if (key == "FORMAT_NVS" || cmd == "FORMAT_NVS") {
        NVSManager::format();
        Serial.println("[CMD] NVS 已格式化! 系统即将重启...");
        delay(500);
        ESP.restart();
    }
    else if (key == "REBOOT" || cmd == "REBOOT") {
        Serial.println("[CMD] 系统即将重启...");
        delay(500);
        ESP.restart();
    }
    else {
        Serial.println("[CMD] 未知命令! 支持: VOUT, IMAX, IMIN, ISTART, DELAY, CALIB_I, NAME, RSTON_TIME, RSTOFF_TIME, OPEN_RST, CLOSE_RST, GET_CFG, FORMAT_NVS, REBOOT");
    }
    
    if (changed) {
        NVSManager::requestSave();
    }
}

static void taskMonitorLoop(void* pvParameters) {
    uint32_t lastPrintTime = 0;
    
    for(;;) {
        if (Serial.available()) {
            String cmd = Serial.readStringUntil('\n');
            handleSerialCommand(cmd);
        }
        
        // 简洁的状态打印 (每秒一次)
        if (millis() - lastPrintTime >= 1000) {
            lastPrintTime = millis();
            
            const char* chgStr = "IDLE";
            if (SystemState::chg_State == CHRG_ING) chgStr = "CHRG";
            else if (SystemState::chg_State == CHRG_FULL) chgStr = "FULL";
            
            const char* faultStr = "NONE";
            if (SystemState::Over_Voltage) faultStr = "OVP";
            if (SystemState::emergency_Stop && SystemState::out_State == OUT_OC) faultStr = "OCP";
            
            Serial.printf("[STATE]BAT=%.2fV, OUT=%.2fV, IOUT=%.3fA, CSO=%.4fV\n",
                SystemState::sensor.battery_voltage,
                SystemState::sensor.output_voltage,
                SystemState::sensor.output_current,
                SystemState::sensor.raw_cso_voltage 
            );
            Serial.printf("[SYS]POWER=%s, CHG=%s, FAULT=%s, NAME=%s\n",
                (SystemState::out_State == OUT_OFF) ? "OFF" : "ON",
                chgStr,
                faultStr,
                SystemState::config.deviceName
            );
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void start_task_monitor() {
    xTaskCreatePinnedToCore(taskMonitorLoop, "Task_Monitor", 4096, NULL, 2, NULL, 1);
}
