#include "Arduino.h"
#include "wire.h"
#include "AHT20.h"


AHT20::AHT20(TwoWire *wire) {
     _wire = wire;
}

//初始化传感器
bool AHT20::begin() {
    delay(40);
    // 2. 检查状态字
    _wire->beginTransmission(AHT20_I2C_ADDR);
    _wire->write(0x71); // 读取状态
    if (_wire->endTransmission() != 0) return false; // I2C设备未找到

    _wire->requestFrom(AHT20_I2C_ADDR, 1);
    if (_wire->available() != 1) return false;
    
    uint8_t status = _wire->read();

    // 发送初始化命令
    if ((status & 0x08) == 0) {
        sendCommand(0xBE, 0x08, 0x00);
        delay(10);
    }
    return true;
}

// 读取数据核心逻辑
bool AHT20::readData(float &temp, float &humi) {
    // 1. 发送测量命令
    sendCommand(0xAC, 0x33, 0x00);

    // 2. 等待测量完成
    delay(80);
    _wire->requestFrom(AHT20_I2C_ADDR, 7);
    if (_wire->available() != 7) return false;

    uint8_t buf[7];
    for (int i = 0; i < 7; i++) {
        buf[i] = _wire->read();
    }

    // 4. 检查 Busy 指示 
    if ((buf[0] & 0x80) != 0) return false; // 传感器忙

    // 5. CRC 校验 
    if (calcCRC8(buf, 6) != buf[6]) return false;

    // 6. 数据解析与转换
    uint32_t raw_hum = ((uint32_t)buf[1] << 12) | ((uint32_t)buf[2] << 4) | ((uint32_t)buf[3] >> 4);
    uint32_t raw_temp = (((uint32_t)buf[3] & 0x0F) << 16) | ((uint32_t)buf[4] << 8) | (uint32_t)buf[5];
    humi = ((float)raw_hum / 1048576.0f) * 100.0f;
    temp = ((float)raw_temp / 1048576.0f) * 200.0f - 50.0f;

    return true;
}

// 内部函数：发送命令
void AHT20::sendCommand(uint8_t cmd, uint8_t d1, uint8_t d2) {
    _wire->beginTransmission(AHT20_I2C_ADDR);
    _wire->write(cmd);
    _wire->write(d1);
    _wire->write(d2);
    _wire->endTransmission();
}

// 内部函数：CRC8 校验 
uint8_t AHT20::calcCRC8(uint8_t *data, uint8_t len) {
    uint8_t crc = 0xFF;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 8; j > 0; --j) {
            if (crc & 0x80) crc = (crc << 1) ^ 0x31;
            else crc = (crc << 1);
        }
    }
    return crc;
}
//逻辑