#ifndef AHT20_DRIVER_H
#define AHT20_DRIVER_H
#include <Arduino.h>
#include <wire.h>
#define AHT20_I2C_ADDR 0x38
#define I2C_SDA_PIN 6
#define I2C_SCL_PIN 7

class AHT20
{
public:
 AHT20(TwoWire *wire = &Wire);

 bool begin();// 初始化传感器

 bool readData(float &temperature, float &humidity);// 读取数据

 private:
 TwoWire *_wire;
 void sendCommand(uint8_t cmd,uint8_t d1, uint8_t d2);   // 发送命令
 uint8_t calcCRC8(uint8_t *data, uint8_t len); //读取数据
};

#endif












