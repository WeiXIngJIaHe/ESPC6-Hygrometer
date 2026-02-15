#ifndef UC8253_DRIVER_H
#define UC8253_DRIVER_H

#include <Arduino.h>
#include <Adafruit_GFX.h>

#define PIN_BUSY  23
#define PIN_RST   22
#define PIN_DC    21
#define PIN_CS    20
#define PIN_SCK   19
#define PIN_MOSI  18

#define DISP_WIDTH_LOGICAL   416
#define DISP_HEIGHT_LOGICAL  240
#define PHYSICAL_WIDTH_X     240
#define PHYSICAL_HEIGHT_Y    416
#define BUFFER_SIZE          (PHYSICAL_WIDTH_X * PHYSICAL_HEIGHT_Y / 8)

#define BLACK 0x00
#define WHITE 0xFF

class EPD_UC8253 : public Adafruit_GFX 
{
public:
    EPD_UC8253();
    ~EPD_UC8253();

    // 初始化 (全刷模式)
    void begin();
    
    // 全刷显示 (更新整个屏幕)
    void display();
    
    // 局刷显示 (更新指定区域)
    // x, y, w, h 为逻辑坐标 (0~416, 0~240)
    void displayPartial();

    // 清空显存 (变白)
    void clearBuffer();
    
    // 深度睡眠
    void deepSleep();
    void initPartial();
    

private:
    uint8_t *frameBuffer;     // 当前帧数据
    uint8_t *oldFrameBuffer;  // 上一帧数据 (用于双缓冲)

    // 底层通信
    void gpioInit();
    void reset();
    void waitBusy();
    void sendCommand(uint8_t cmd);
    void sendData(uint8_t data);
    void writeByte(uint8_t data);
    void drawPixel(int16_t x, int16_t y, uint16_t color);
    
    // 局刷专用初始化 (魔法指令)
    
    
    // 设置局刷窗口 helper
    void setPartialWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
    void setRamPointer(uint8_t x, uint16_t y);
};
#endif