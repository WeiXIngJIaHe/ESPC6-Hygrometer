#include <Arduino.h>
#include "UC8253.h"
#include "AHT20.h"
#include <Wire.h>
#include "ICP20100.h"
#include <GUI.h>
#include "Font.h"
#include "const_time.h"

#define DATA_X_START 250
#define TEMP_Y       50
#define HUMI_Y       120
#define pressure_Y   220
#define ERASE_W      120  // 擦除宽度 (足够覆盖数字即可)
#define ERASE_H      60  // 擦除高度 (对应 TextSize 2 的高度)

struct SystemData//结构体初始化
{
float Temp;
float Humi;
float Pressure;
bool timeValid;
};

// 实例化传感器对象
AHT20 aht;
ICP20100 icp;
EPD_UC8253 epd; 
SystemData sysData;

void drawUserInterface() 
{
    // 4. 显示温度和湿度
     epd.clearBuffer(); // 清空缓存
     epd.drawBitmap(200,14, TEMP, 48, 50, 1,0); // 绘制温度图标
     epd.drawBitmap(210,80, HUMI, 30, 54, 1,0); // 绘制湿度图标
    epd.drawBitmap(0,0,Background,184,240,1,0); // 绘制背景图片 
    // 显示时间
    epd.setFont(&Rajdhani_Light_612pt7b);
    epd.setTextSize(1); 
    epd.setTextColor(0); // 黑色字
    epd.setCursor(245,230); // 打印气压
    /*epd.print("200"); // 摄氏温度显示
    epd.print("KPa"); // 单位符号*/
}

void updateSensors(SystemData &data) 
{
     // 1. 读取 AHT20
    float t_aht, h_aht;
    bool aht_ok = aht.readData(t_aht, h_aht);
    if (aht_ok) 
    {
        data.Temp = t_aht;
        data.Humi = h_aht;
    }
}

void UPTime()
{
    epd.initPartial(); // 初始化局部刷新模式
     if(checkMinuteChanged())
     {
        String clockStr = getClockString();
        epd.fillRect(220,145,150, 60, 1); // 擦除旧时间 
        epd.setFont(&Rajdhani_Light_618pt7b);
        epd.setTextColor(0); // 黑色字
        epd.setTextSize(2);
        epd.setCursor(220,190);
        epd.print(clockStr); 
        epd.display(); // 局部刷新显示
     }

      if (WiFi.status() == WL_CONNECTED)
      {
            
            epd.drawBitmap(370,222, WIFI, 20, 20, 1,0); // 画 WiFi 图标
            epd.display(); // 局部刷新显示
        } 
        else 
        {
            epd.fillRect(370,220, 20, 20, 1); // 擦除 WiFi 图标区域 
            epd.drawBitmap(370,220, WIFI_NOT, 20, 20, 1,0); // 擦除 WiFi 图标区域
            epd.display(); // 局部刷新显示
        }
    
}

void updateDataUI(const SystemData &data)
{   
    epd.initPartial(); // 初始化局部刷新模式
    // 擦除旧数据 
    
    epd.fillRect(DATA_X_START, TEMP_Y-25, ERASE_W, ERASE_H, 1); 
    epd.fillRect(DATA_X_START, HUMI_Y-25, ERASE_W, 40, 1);

    // --- B. 写入新数据 ---
    epd.setTextColor(0); // 黑色字
    epd.setTextSize(1);
    epd.setFont(&Rajdhani_Light_618pt7b);

    // 打印温度
    epd.setCursor(DATA_X_START, TEMP_Y);
    epd.print(data.Temp + 273.15);// 摄氏转开氏温度显示
    epd.print("K");

    // 打印湿度
    epd.setCursor(DATA_X_START, HUMI_Y);
    epd.print(data.Humi);
    epd.print("%"); 
    epd.display(); // 局部刷新显示
}

void updrawface()
{
   epd.initPartial(); // 初始化局部刷新模式
   if (sysData.Temp >= 20 && sysData.Temp <= 30 && sysData.Humi >= 40 && sysData.Humi <= 60) // 温度和湿度在舒适范围内
   {
      epd.drawBitmap(83,69, EYE_Close, 20, 17, 1,0); // 绘制温度图标
      epd.display();
   }
   else // 温度和湿度不在舒适范围内 
   {
    epd.drawBitmap(83,69, EYE_OPEN, 20, 17, 1,0); // 绘制温度图标
    epd.display();
   }
}

void setup() 
{
    Serial.begin(115200);// 启动串口调试
    delay(1000);  // 等待串口稳定
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN); 

    // 检查传感器
    if (!aht.begin()) 
    {
        Serial.println("AHT20 not found!");
    }

    setupTimeSystem(ssid, password); // 设置 WiFi 并同步时间
    epd.begin(); // 初始化 EPD
    epd.clearBuffer(); // 清空缓存
    epd.fillScreen(1); // 清屏
    drawUserInterface();  
    epd.display(); // 显示更新
    updateSensors(sysData); // 获取新传感器数据        
    updateDataUI(sysData); // 更新显示数据
    WiFi.setSleep(false);
    UPTime();
}

void loop() 
{
    delay(10000);
    updateSensors(sysData); // 获取新传感器数据        
    updateDataUI(sysData); 
    updrawface();
    UPTime();
}



      






            
        
