#include "WiFi.h"// 引入 WiFi 库
#include "time.h"
#include "esp_sntp.h"

const char* ntpServer1 = "pool.ntp.org"; // 首选时间服务器
const char* ntpServer2 = "time.nist.gov"; // 备用
const long  gmtOffset_sec = 8 * 3600;     // 时区偏移：东八区 (8 * 3600)
const int   daylightOffset_sec = 0;       // 夏令时偏移：无
const char* ssid     = "JiaHe";
const char* password = "12345678";

// 全局时间结构体
struct tm timeinfo;
bool isTimeSynced = false; 
void WiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) 
{
    switch(event) 
    {
        case ARDUINO_EVENT_WIFI_STA_START:
            Serial.println("WiFi Station Started");
            break;
            
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            Serial.println("Connected to Access Point (Handshake needed)");
            break;

        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            Serial.print("IP Address: ");
            Serial.println(WiFi.localIP());
            break;

        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            uint8_t reason = info.wifi_sta_disconnected.reason;
            Serial.println("Disconnected!");
            Serial.print("Reason Code: [ ");
            Serial.print(reason);
            Serial.println(" ]");
            
            // --- 现场分析罪魁祸首 ---
            if (reason == 202 || reason == 15) {
                Serial.println("密码错误");
            } else if (reason == 201) {
                Serial.println("找不到热点 ");
            } else if (reason == 2) {
                Serial.println("认证过期 ");
            }
            
            // 简单的慢速重连，方便观察
            delay(3000);
            WiFi.begin(ssid, password);
            break;
        }
}

//时间同步回调函数 
void timeSyncCallback(struct timeval *t) {
    Serial.println("[NTP] Time Updated/Synchronized!");
    isTimeSynced = true;
}

//初始化函数
void setupTimeSystem(const char* ssid, const char* pwd) {
    // 设置 Wi-Fi 回调
    WiFi.onEvent(WiFiEvent);
    
    // 连接 Wi-Fi
    WiFi.begin(ssid, pwd);
    Serial.print("Connecting to WiFi");
    
    // 设置时间同步回调
    sntp_set_time_sync_notification_cb(timeSyncCallback);

    // 配置 NTP
    
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);
}

// --- 4. 获取格式化时间字符串 ---
String getClockString() {
    if(!getLocalTime(&timeinfo)){
        return "--:--"; 
    }
    char timeStr[10];
    sprintf(timeStr, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
    return String(timeStr);
}

// 检查是否需要刷新 
bool checkMinuteChanged() {
    static int last_min = -1;
    if(!getLocalTime(&timeinfo)) return false;
    
    if (timeinfo.tm_min != last_min) {
        last_min = timeinfo.tm_min;
        return true;
    }
    return false;
}