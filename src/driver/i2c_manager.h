#pragma once

#include "global.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

/**
 * @brief I2C总线管理类
 *
 * 功能:
 * 1. 初始化ESP32-S3 I2C外设
 * 2. 提供多任务互斥访问保护
 * 3. 防止ADS1115和SC8726A同时访问总线
 * 4. 提供总线死锁恢复机制
 */
class I2CManager
{
public:

    /**
     * @brief 初始化I2C
     *
     * @return true 初始化成功
     * @return false 初始化失败
     */
    static bool begin();

    /**
     * @brief 获取I2C锁
     *
     * @param timeout_ms 等待时间
     * @return true 成功获取锁
     * @return false 获取锁超时
     */
    static bool lock(uint32_t timeout_ms);

    /**
     * @brief 释放I2C锁
     */
    static void unlock();

    /**
     * @brief 恢复I2C死锁状态 (发送9个时钟脉冲)
     *
     * @return true 恢复成功
     * @return false 恢复失败
     */
    static bool recover();

    /**
     * @brief 扫描I2C设备
     *
     * @return uint8_t 在线设备数量
     */
    static uint8_t scan();

private:
    static SemaphoreHandle_t mutex;
    static bool initialized;
};
