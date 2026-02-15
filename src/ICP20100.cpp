#include "ICP20100.h"

ICP20100::ICP20100(uint8_t addr) {
    _addr = addr;
}

bool ICP20100::begin(TwoWire &wirePort) {
    _wire = &wirePort;
    _wire->begin();

    // 1. 简单的 I2C 通信测试
    _wire->beginTransmission(_addr);
    if (_wire->endTransmission() != 0) {
        return false; // I2C 总线找不到设备
    }

    // 2. 检查 Device ID 
    if (!isConnected()) {
        return false; // ID 不匹配
    }

    // 3. 检查 Boot Status (针对 Version A 的简单检查) [cite: 1207]
    // 注意：完整 Version A Boot Sequence 非常复杂，这里假设是 Version B 
    // 或者 Version A 已经完成了内部 Boot。
    // 如果读取数据全是 0，可能需要执行完整的 OTP Boot (参考手册 6.5 节 [cite: 786])
    
    // 4. 启动测量
    startContinuousMode();

    return true;
}

bool ICP20100::isConnected() {
    uint8_t id = readRegister(REG_DEVICE_ID);
    // ID 必须是 0x63 
    return (id == 0x63);
}

void ICP20100::startContinuousMode() {
    // 寄存器 0xC0 (MODE_SELECT) 配置:
    // Bit 6: Power Mode = 1 (Active Mode)
    // Bit 3: Meas Mode = 1 (Continuous)
    // Bit 2-0: Meas Config = 000 (Mode 0: Low Noise, 25Hz ODR) [cite: 1223]
    
    // Value = 0b01001000 = 0x48
    writeRegister(REG_MODE_SELECT, 0x48);
    
    // 等待模式同步完成 [cite: 895]
    // 检查 DEVICE_STATUS (0xCD) 的 Bit 0 是否为 1
    // 简单延时代替轮询，确保稳定
    delay(10); 
}

bool ICP20100::getData(float &pressure_kPa, float &temperature_C) {
    // 读取 6 个字节的数据: Press_0, Press_1, Press_2, Temp_0, Temp_1, Temp_2
    // 地址从 0xFA 开始连续读取 [cite: 900]
    uint8_t buffer[6];
    readBurst(REG_PRESS_DATA_0, buffer, 6);

    // --- 压力计算 ---
    // buffer[0]=LSB, buffer[1]=Mid, buffer[2]=MSB (Low nibble) [cite: 1305-1315]
    // 注意：Press_Data_2 只有低4位有效 [19:16]
    int32_t p_raw = convert20BitSigned(buffer[2], buffer[1], buffer[0]);
    
    // 公式: P = (P_out / 2^17) * 40kPa + 70kPa [cite: 908]
    pressure_kPa = ((float)p_raw / 131072.0f) * 40.0f + 70.0f;

    // --- 温度计算 ---
    // buffer[3]=LSB, buffer[4]=Mid, buffer[5]=MSB (Low nibble) [cite: 1321-1335]
    int32_t t_raw = convert20BitSigned(buffer[5], buffer[4], buffer[3]);

    // 公式: T = (T_out / 2^18) * 65C + 25C [cite: 926]
    temperature_C = ((float)t_raw / 262144.0f) * 65.0f + 25.0f;

    return true;
}

// 辅助：将3个字节拼成20位有符号整数
int32_t ICP20100::convert20BitSigned(uint8_t msb, uint8_t mid, uint8_t lsb) {
    // 拼接：MSB(低4位) | MID | LSB
    int32_t val = ((int32_t)(msb & 0x0F) << 16) | ((int32_t)mid << 8) | lsb;

    // 符号扩展 (Sign Extension)
    // 如果第20位 (Bit 19) 是 1，说明是负数，需要把高位全填 1
    if (val & 0x00080000) {
        val |= 0xFFF00000;
    }
    return val;
}

void ICP20100::writeRegister(uint8_t reg, uint8_t val) {
    _wire->beginTransmission(_addr);
    _wire->write(reg);
    _wire->write(val);
    _wire->endTransmission();
}

uint8_t ICP20100::readRegister(uint8_t reg) {
    _wire->beginTransmission(_addr);
    _wire->write(reg);
    _wire->endTransmission(false);
    _wire->requestFrom(_addr, (uint8_t)1);
    if (_wire->available()) return _wire->read();
    return 0;
}

void ICP20100::readBurst(uint8_t startReg, uint8_t *buffer, uint8_t len) {
    _wire->beginTransmission(_addr);
    _wire->write(startReg);
    _wire->endTransmission(false);
    _wire->requestFrom(_addr, len);
    for (int i = 0; i < len; i++) {
        if (_wire->available()) buffer[i] = _wire->read();
        else buffer[i] = 0;
    }
}