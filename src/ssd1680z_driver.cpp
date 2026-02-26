#include "ssd1680z_driver.h"

// 构造函数：告诉 GFX 库我们真实的物理尺寸 (250x122)
EPD_SSD1680::EPD_SSD1680() : Adafruit_GFX(DISP_WIDTH_PHYSICAL, DISP_HEIGHT_PHYSICAL) {
    // 构造时什么都不做
}

EPD_SSD1680::~EPD_SSD1680() {}
// --- 底层通信 (软件SPI) ---
void EPD_SSD1680::gpioInit() {
    pinMode(PIN_BUSY, INPUT);
    pinMode(PIN_RST, OUTPUT);
    pinMode(PIN_DC, OUTPUT);
    pinMode(PIN_CS, OUTPUT);
    pinMode(PIN_SCK, OUTPUT);
    pinMode(PIN_MOSI, OUTPUT);
    digitalWrite(PIN_CS, HIGH);
    digitalWrite(PIN_SCK, LOW);
}

void EPD_SSD1680::writeByte(uint8_t data) {
    for (int i = 0; i < 8; i++) {
        digitalWrite(PIN_SCK, LOW);
        if (data & 0x80) digitalWrite(PIN_MOSI, HIGH);
        else             digitalWrite(PIN_MOSI, LOW);
        data <<= 1;
        digitalWrite(PIN_SCK, HIGH);
    }
    digitalWrite(PIN_SCK, LOW);
}

void EPD_SSD1680::sendCommand(uint8_t cmd) {
    digitalWrite(PIN_DC, LOW);
    digitalWrite(PIN_CS, LOW);
    writeByte(cmd); 
    digitalWrite(PIN_CS, HIGH);
}

void EPD_SSD1680::sendData(uint8_t data) {
    digitalWrite(PIN_DC, HIGH); 
    digitalWrite(PIN_CS, LOW);
    writeByte(data); 
    digitalWrite(PIN_CS, HIGH);
}

void EPD_SSD1680::waitBusy() {
    unsigned long start = millis();
    delay(100);
    while (digitalRead(PIN_BUSY) == HIGH) {
        if (millis() - start > 5000) break; // 2秒超时防止死机
        delay(10);
    }
    delay(100); // 等待稳定
}

void EPD_SSD1680::reset()  // 硬件复位 (30ms)
{
    digitalWrite(PIN_RST, HIGH); delay(20); 
    digitalWrite(PIN_RST, LOW);  delay(20); 
    digitalWrite(PIN_RST, HIGH); delay(20);
    waitBusy();
}

// --- 核心逻辑 ---

void EPD_SSD1680::begin()  // 初始化流程
{
    gpioInit();
    reset();
    waitBusy();
    sendCommand(0x12); waitBusy(); // 软复位

    sendCommand(0x01); // 驱动设置
    sendData(0xF9);    // 250行
    sendData(0x00); sendData(0x00);

    sendCommand(0x11); // 数据进入模式 X+, Y+
    sendData(0x03);

    // RAM X 设置 (0~15 = 128像素) -> 解决错位
    sendCommand(0x44); sendData(0x00); sendData(0x0F);
    
    // RAM Y 设置 (0~249)
    sendCommand(0x45); 
    sendData(0x00); sendData(0x00);
    sendData(0xF9); sendData(0x00);

    sendCommand(0x3C); sendData(0x33); // 边框设置 (强制白边)

    // 屏蔽红色层 (解决斑点)
    sendCommand(0x21); sendData(0x00); sendData(0x80); 
}

void EPD_SSD1680::display()  
{
    // 1. 写黑白数据
    sendCommand(0x24);
    for (int i = 0; i < BUFFER_SIZE; i++) {
        sendData(frameBuffer[i]);
    }

    // 2. 写红色数据 (全填FF透明) -> 双重保险防斑点
    sendCommand(0x26);
    for (int i = 0; i < BUFFER_SIZE; i++) {
        sendData(0xFF);
    }

    // 3. 刷新
    sendCommand(0x22); sendData(0xF7);
    sendCommand(0x20); waitBusy();
}

void EPD_SSD1680::setRamPointer(uint8_t addrX, uint16_t addrY) {
    sendCommand(0x4E); // Set RAM X Address Counter
    sendData(addrX);
    
    sendCommand(0x4F); // Set RAM Y Address Counter
    sendData(addrY & 0xFF);
    sendData((addrY >> 8) & 0xFF);
}





void EPD_SSD1680::displayPartial() 
{
       setRamPointer(0x00, 0xF9); 

    // 2. 写入图像数据
    sendCommand(0x24);
    for (int i = 0; i < BUFFER_SIZE; i++) {
        sendData(frameBuffer[i]);
    }

    // --- 新增：加载局刷 LUT ---
    // 告诉芯片：别用你自带的烂波形了，用我给你的这个！
    sendCommand(0x32); 
    for (int i = 0; i < sizeof(lut_partial); i++) {
        sendData(lut_partial[i]);
    }
    // -------------------------

    // 3. 刷新指令
    // 注意：加载了自定义 LUT 后，通常使用 0xC7 或 0xFF 来激活
 
    sendCommand(0x22);
    sendData(0xFF); // 或者 0xC7，取决于 LUT 的具体定义
    
    // 4. 激活
    sendCommand(0x20);
    waitBusy();
   

}

void EPD_SSD1680::clearBuffer() {
    memset(frameBuffer, 0xFF, BUFFER_SIZE); // 0xFF = 白
}

void EPD_SSD1680::deepSleep() {
    sendCommand(0x10); sendData(0x01);
}

