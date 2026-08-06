#pragma once

#include "global.h"

/**
 * @brief SC8726A 升降压芯片驱动类
 * 
 * 功能:
 * 1. 初始化 CE 引脚
 * 2. 动态调节输出电压寄存器
 * 3. 硬件启停控制
 */
class SC8726A_Driver {
public:
    /**
     * @brief 初始化驱动，配置CE为输出并默认拉低
     * 
     * @return true 初始化成功
     */
    static bool begin();

    /**
     * @brief 使能或关闭芯片硬件输出
     * 
     * @param state true=开启，false=关闭
     */
    static void setCE(bool state);

    /**
     * @brief 动态设置芯片输出目标电压
     * 
     * @param targetV 目标电压(V)，最小为5.0V
     * @return true 设置成功
     * @return false 设置失败(例如获取不到总线锁)
     */
    static bool setVoltage(float targetV);
};
