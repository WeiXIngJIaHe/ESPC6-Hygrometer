#include "UC8253.h"
EPD_UC8253::EPD_UC8253() : Adafruit_GFX(DISP_WIDTH_LOGICAL, DISP_HEIGHT_LOGICAL) {

    frameBuffer = (uint8_t*)malloc(BUFFER_SIZE);
    oldFrameBuffer = (uint8_t*)malloc(BUFFER_SIZE);
    
    if (frameBuffer) memset(frameBuffer, 0xFF, BUFFER_SIZE);
    if (oldFrameBuffer) memset(oldFrameBuffer, 0xFF, BUFFER_SIZE); // 初始化为白色
}

EPD_UC8253::~EPD_UC8253() {
    if (frameBuffer) free(frameBuffer);
    if (oldFrameBuffer) free(oldFrameBuffer);
}//··底层逻辑

// --- 底层通信实现 ---
void EPD_UC8253::gpioInit() 
{
    pinMode(PIN_BUSY, INPUT); 
    pinMode(PIN_RST, OUTPUT);
    pinMode(PIN_DC, OUTPUT);
    pinMode(PIN_CS, OUTPUT);
    pinMode(PIN_SCK, OUTPUT);
    pinMode(PIN_MOSI, OUTPUT);
    digitalWrite(PIN_CS, HIGH);
    digitalWrite(PIN_SCK, LOW);
}

void EPD_UC8253::writeByte(uint8_t data) {
    for (int i = 0; i < 8; i++) {
        digitalWrite(PIN_SCK, LOW);
        if (data & 0x80) digitalWrite(PIN_MOSI, HIGH);
        else             digitalWrite(PIN_MOSI, LOW);
        data <<= 1;
        digitalWrite(PIN_SCK, HIGH);
    }
}

void EPD_UC8253::sendCommand(uint8_t cmd) {
    digitalWrite(PIN_DC, LOW);
    digitalWrite(PIN_CS, LOW);
    writeByte(cmd);
    digitalWrite(PIN_CS, HIGH);
}
//发送字节
void EPD_UC8253::sendData(uint8_t data) {
    digitalWrite(PIN_DC, HIGH);
    digitalWrite(PIN_CS, LOW);
    writeByte(data);
    digitalWrite(PIN_CS, HIGH);
}

void EPD_UC8253::waitBusy() {
    // 0 (Low) = Busy, 1 (High) = Free
    unsigned long start = millis();
    while (digitalRead(PIN_BUSY) == LOW) {
        if (millis() - start > 5000) break; 
        delay(1);
    }
}

void EPD_UC8253::reset() {
    // 复刻 EPD_HW_RESET
    digitalWrite(PIN_RST, LOW);  delay(10); 
    digitalWrite(PIN_RST, HIGH); delay(10); 
    waitBusy();
}

// 全刷初始化

void EPD_UC8253::begin() {
    gpioInit();
    reset();
    sendCommand(0x00); 
    sendData(0x1B); 
    sendCommand(0x44); // Set RAM X Start/End 
    sendData(0x00); // Start: 0 sendData(0x1D); // End: 29 (30 bytes = 240 pixels) 
    sendCommand(0x45); // Set RAM Y Start/End 
    sendData(0x00); sendData(0x00); // Start: 0 
    sendData(0xA0); sendData(0x01); 
}

//全刷显示
void EPD_UC8253::display() {
    if (!frameBuffer || !oldFrameBuffer) return;
    setRamPointer(0,0);
    sendCommand(0x10);
    for (uint32_t i = 0; i < BUFFER_SIZE; i++) {
        sendData(oldFrameBuffer[i]);
    }

  
    setRamPointer(0, 0);
    sendCommand(0x13);
    for (uint32_t i = 0; i < BUFFER_SIZE; i++) {
        sendData(frameBuffer[i]);
        // 同步更新旧缓存
        oldFrameBuffer[i] = frameBuffer[i]; 
    }

    
    sendCommand(0x04);//发送寄存器
    waitBusy();

  
    sendCommand(0x12);
    waitBusy();
    
    
    sendCommand(0x02);
    waitBusy();
}

//局刷初始化 

void EPD_UC8253::initPartial() {
    reset(); // 局刷前先硬复位，保证状态纯净

    waitBusy();// 等待 BUSY 清零，确保复位完成

    // 面板设置 (保持一致)
    sendCommand(0x00); 
    sendData(0x1B);

    // 开启电源
    sendCommand(0x04);
    waitBusy();

  
    sendCommand(0xE0); sendData(0x02); //开启电源
    sendCommand(0xE5); sendData(0x6E); 
    
   
    sendCommand(0x50); sendData(0xD7); 
}

// 辅助函数：设置硬件窗口
void EPD_UC8253::setPartialWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    
    uint16_t x_start = x & 0xF8;
    uint16_t x_end = x_start + w - 1;
    // 补齐结束位
    x_end |= 0x07; 

    sendCommand(0x91); // 进入局刷模式
    sendCommand(0x90); // 设置窗口
    
    
    sendData(x_start / 8); 
    sendData(x_end / 8);
    sendData(y >> 8); 
    sendData(y & 0xFF);
    sendData((y + h - 1) >> 8); 
    sendData((y + h - 1) & 0xFF);
    sendData(0x01); // PT_SCAN
}

// 刷执行 
void EPD_UC8253::displayPartial() 
{
    reset();
    waitBusy();
    sendCommand(0x00); // Panel Setting
    sendData(0x1B); // Panel Setting
    sendCommand(0XE0);
    sendData(0x02); 
    sendCommand(0xE5);
    sendData(0x5F); 

}

void EPD_UC8253::clearBuffer() {
    if (frameBuffer) memset(frameBuffer, 0xFF, BUFFER_SIZE);
}

void EPD_UC8253::deepSleep() {
    sendCommand(0x02); // Power OFF
    waitBusy();
    sendCommand(0x07); // Deep Sleep
    sendData(0xA5);
}

void EPD_UC8253::setRamPointer(uint8_t x, uint16_t y) 
{
sendCommand(0x4E); // 设置数据写入位置
sendData(x); // X 坐标 (0-239)
sendCommand(0x4F); // 设置 Y 坐标高位
sendData(y & 0xFF);
sendData(y >> 8); // Y 低位
}

void EPD_UC8253::drawPixel(int16_t x, int16_t y, uint16_t color) {
    // 1. 逻辑坐标范围检查 (横屏范围)
    if (x < 0 || x >= 416 || y < 0 || y >= 240) return;

    // 2. 坐标旋转映射 (90度旋转)
    
    int16_t y_phy = (PHYSICAL_WIDTH_X - 1) - x; // (239 - y)
    int16_t x_phy = y;                          // x 直接变 y
     y_phy = 239 -y_phy; 
   
    int32_t byteIdx = y_phy * 30 + (x_phy / 8);

    if (byteIdx >= 0 && byteIdx < BUFFER_SIZE) {

        uint8_t bitMask = 0x80 >> (x_phy % 8);

        if (color == 1) frameBuffer[byteIdx] |= bitMask;  // 白
        else            frameBuffer[byteIdx] &= ~bitMask; // 黑
    }
}