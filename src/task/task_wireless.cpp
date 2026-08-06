#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>

#include "../include/global.h"
#include "../include/ota_html.h"
#include "../core/system_state.h"
#include "../driver/sc8726a.h"

static WebServer server(80);

static void setupOTAWebServer() {
    server.on("/", HTTP_GET, []() {
        // 返回升级网页，并动态注入版本号
        server.sendHeader("Connection", "close");
        String html = String(serverIndex);
        html.replace("{{CURRENT_VERSION}}", FIRMWARE_VERSION_STR);
        server.send(200, "text/html", html);
    });

    server.on("/update", HTTP_POST, []() {
        // 升级结束后的请求返回
        server.sendHeader("Connection", "close");
        server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
        if (!Update.hasError()) {
            delay(500);
            ESP.restart();
        }
    }, []() {
        // 升级过程中的流式接收处理
        HTTPUpload& upload = server.upload();
        if (upload.status == UPLOAD_FILE_START) {
            // 为了保证刷写固件时的绝对安全，强行切断底层供电输出
            SystemState::out_State = OUT_OFF;
            SystemState::emergency_Stop = true;
            SC8726A_Driver::setVoltage(0.0f);
            SC8726A_Driver::setCE(false);
            
            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { 
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            // 续命，保证上传大文件时不会超时
            SystemState::wireless_ActiveTime = millis();
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            if (Update.end(true)) { 
            } else {
                Update.printError(Serial);
            }
        }
    });
    
    server.begin();
}

static void taskWirelessLoop(void* pvParameters) {
    bool wifi_initialized = false;
    
    for(;;) {
        // ================= 状态机：按键开启进行 OTA 热点 =================
        if (SystemState::Wireless_Enabled && !wifi_initialized) {
            WiFi.mode(WIFI_AP);
            WiFi.setTxPower(WIFI_POWER_19_5dBm); // 最大发射功率保证直连速度
            
            // 使用设置里的设备名作为热点名称
            String apSSID = String(SystemState::config.deviceName) + "_OTA";
            WiFi.softAP(apSSID.c_str(), "12345678");
            Serial.printf("[WIFI] SSID: %s\n", apSSID.c_str());
            Serial.printf("[WIFI] PASS: 12345678\n");
            Serial.printf("[WIFI]UPdata WEB:192.168.4.1\n");
            
            setupOTAWebServer();
            
            SystemState::wireless_ActiveTime = millis();
            wifi_initialized = true;
        }
        
        // ================= 状态机：关闭与超时注销 =================
        if (!SystemState::Wireless_Enabled && wifi_initialized) {
            Serial.println("[WIFI] connet timeout");
            
            server.stop();
            WiFi.softAPdisconnect(true);
            WiFi.mode(WIFI_OFF);
            
            wifi_initialized = false;
            Serial.println("[WIFI] OFF。");
        }
        
        // ================= 正常运行时的循环处理 =================
        if (wifi_initialized) {
            // 超时检测 (10 分钟 = 600,000 毫秒)
            if (millis() - SystemState::wireless_ActiveTime > 600000) {
                SystemState::Wireless_Enabled = false;
                continue; // 直接进行下一轮循环，进入关闭逻辑
            }
            
            // 非阻塞处理手机网页请求
            server.handleClient();
        }
        
        // 降低延迟，防止看门狗复位
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void start_task_wireless() {
    // 固件写入需要较大的栈空间
    xTaskCreatePinnedToCore(taskWirelessLoop, "Task_Wireless", 8192, NULL, 1, NULL, 0); 
}
