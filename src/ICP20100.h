#ifndef ICP20100_H
#define ICP20100_H

#include <Arduino.h>
#include <Wire.h>

// I2C 地址: ADO 接地为 0x63, 接 VDDIO 为 0x64
#define ICP20100_I2C_ADDR_0 0x63
#define ICP20100_I2C_ADDR_1 0x64

// 寄存器地址定义 (基于 Datasheet Table 21)
#define REG_TRIM1_MSB       0x05
#define REG_TRIM2_LSB       0x06
#define REG_TRIM2_MSB       0x07
#define REG_DEVICE_ID       0x0C  // 应为 0x63
#define REG_IO_DRIVE_STR    0x0D
#define REG_OTP_CONFIG1     0xAC
#define REG_OTP_STATUS2     0xBF
#define REG_MODE_SELECT     0xC0  // 关键控制寄存器
#define REG_INT_STATUS      0xC1
#define REG_INT_MASK        0xC2
#define REG_FIFO_CONFIG     0xC3
#define REG_FIFO_FILL       0xC4
#define REG_PRESS_ABS_LSB   0xC7
#define REG_DEVICE_STATUS   0xCD
#define REG_VERSION         0xD3  // 版本号
#define REG_PRESS_DATA_0    0xFA  // 压力数据起始
#define REG_TEMP_DATA_0     0xFD  // 温度数据起始

class ICP20100 {
public:
    ICP20100(uint8_t addr = ICP20100_I2C_ADDR_0);

    // 初始化：检查 ID 并启动传感器
    bool begin(TwoWire &wirePort = Wire);

    // 检查是否连接
    bool isConnected();

    // 启动连续测量模式 (Mode 0: Low Noise)
    void startContinuousMode();

    // 读取物理数据 (返回 true 表示读取成功)
    bool getData(float &pressure_kPa, float &temperature_C);// 返回压力和温度数据，单位分别为 kPa 和 °C

    // 软复位 (可选)
    void softReset();

private:
    uint8_t _addr;
    TwoWire *_wire;

    void writeRegister(uint8_t reg, uint8_t val);
    uint8_t readRegister(uint8_t reg);
    void readBurst(uint8_t startReg, uint8_t *buffer, uint8_t len);
    
    // 20-bit 补码转换辅助函数
    int32_t convert20BitSigned(uint8_t msb, uint8_t mid, uint8_t lsb);
};

#endif