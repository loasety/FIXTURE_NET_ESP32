#pragma once

#include "global.h"

class NVSManager {
public:
    static void init();
    static void requestSave();
    static void taskNvsLoop(void* pvParameters);

private:
    static volatile bool saveRequested;
    static uint32_t lastRequestTime;
};

extern void start_task_nvs();
