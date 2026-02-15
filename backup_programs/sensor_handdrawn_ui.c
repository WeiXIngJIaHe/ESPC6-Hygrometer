/**
 * @file sensor_handdrawn_ui.c
 * @brief 手写UI传感器显示程序 - 美观设计
 * @note 不使用LVGL，直接绘制图形界面
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "ssd1680z_driver.h"
#include "I2C_driver.h"
#include "AHT20_temp_driver.h"
#include "ags10_driver.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "SENSOR_UI";

// 5x7点阵字体数据（ASCII 32-126）
static const uint8_t font_5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // 空格
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // !
    {0x00, 0x07, 0x00, 0x07, 0x00}, // "
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // #
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // %
    {0x36, 0x49, 0x55, 0x22, 0x50}, // &
    {0x00, 0x05, 0x03, 0x00, 0x00}, // '
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // (
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // )
    {0x08, 0x2A, 0x1C, 0x2A, 0x08}, // *
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // +
    {0x00, 0x50, 0x30, 0x00, 0x00}, // ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // -
    {0x00, 0x60, 0x60, 0x00, 0x00}, // .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // /
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
    {0x00, 0x36, 0x36, 0x00, 0x00}, // :
    {0x00, 0x56, 0x36, 0x00, 0x00}, // ;
    {0x00, 0x08, 0x14, 0x22, 0x41}, // <
    {0x14, 0x14, 0x14, 0x14, 0x14}, // =
    {0x41, 0x22, 0x14, 0x08, 0x00}, // >
    {0x02, 0x01, 0x51, 0x09, 0x06}, // ?
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // @
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
    {0x7F, 0x09, 0x09, 0x01, 0x01}, // F
    {0x3E, 0x41, 0x41, 0x51, 0x32}, // G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
    {0x7F, 0x02, 0x04, 0x02, 0x7F}, // M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // S
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
    {0x7F, 0x20, 0x18, 0x20, 0x7F}, // W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // X
    {0x03, 0x04, 0x78, 0x04, 0x03}, // Y
    {0x61, 0x51, 0x49, 0x45, 0x43}, // Z
};

// 绘制单个字符（5x7点阵，放大2倍=10x14）
void draw_char_large(uint16_t x, uint16_t y, char c) {
    if (c < ' ' || c > 'Z') c = ' ';
    const uint8_t *glyph = font_5x7[c - ' '];

    for (int col = 0; col < 5; col++) {
        for (int row = 0; row < 7; row++) {
            if (glyph[col] & (1 << row)) {
                // 放大2倍：每个点变成2x2
                eink_set_pixel(x + col * 2, y + row * 2, 0);
                eink_set_pixel(x + col * 2 + 1, y + row * 2, 0);
                eink_set_pixel(x + col * 2, y + row * 2 + 1, 0);
                eink_set_pixel(x + col * 2 + 1, y + row * 2 + 1, 0);
            }
        }
    }
}

// 绘制字符串（10x14字符，间距2像素）
void draw_string(uint16_t x, uint16_t y, const char *str) {
    uint16_t cursor_x = x;
    while (*str) {
        draw_char_large(cursor_x, y, *str);
        cursor_x += 12;  // 10px宽度 + 2px间距
        str++;
    }
}

// 绘制横线
void draw_hline(uint16_t x, uint16_t y, uint16_t len) {
    for (uint16_t i = 0; i < len; i++) {
        eink_set_pixel(x + i, y, 0);
    }
}

// 绘制竖线
void draw_vline(uint16_t x, uint16_t y, uint16_t len) {
    for (uint16_t i = 0; i < len; i++) {
        eink_set_pixel(x, y + i, 0);
    }
}

// 绘制矩形框
void draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    draw_hline(x, y, w);
    draw_hline(x, y + h - 1, w);
    draw_vline(x, y, h);
    draw_vline(x + w - 1, y, h);
}

// 绘制实心圆点（用于装饰）
void draw_dot(uint16_t x, uint16_t y, uint16_t r) {
    for (int dy = -r; dy <= r; dy++) {
        for (int dx = -r; dx <= r; dx++) {
            if (dx * dx + dy * dy <= r * r) {
                eink_set_pixel(x + dx, y + dy, 0);
            }
        }
    }
}

// 绘制完整UI界面
void draw_ui(float temp, float humidity, uint32_t tvoc, uint32_t uptime) {
    uint8_t *buffer = eink_get_buffer();

    // 清空缓冲区
    memset(buffer, 0xFF, (122 * 250) / 8);

    // === 顶部标题区 ===
    draw_string(20, 8, "ESP32-C6");
    draw_string(25, 22, "Sensors");
    draw_hline(10, 40, 102);

    // === 温度区域 ===
    draw_dot(15, 58, 2);  // 装饰圆点
    draw_string(23, 52, "Temp");
    char temp_str[16];
    snprintf(temp_str, sizeof(temp_str), "%.1fC", temp);
    draw_string(20, 68, temp_str);

    // === 湿度区域 ===
    draw_dot(15, 98, 2);  // 装饰圆点
    draw_string(23, 92, "Humi");
    char humi_str[16];
    snprintf(humi_str, sizeof(humi_str), "%.1f%%", humidity);
    draw_string(20, 108, humi_str);

    // === TVOC区域 ===
    draw_dot(15, 138, 2);  // 装饰圆点
    draw_string(23, 132, "TVOC");
    char tvoc_str[16];
    snprintf(tvoc_str, sizeof(tvoc_str), "%luppb", tvoc);
    draw_string(20, 148, tvoc_str);

    // === 分隔线 ===
    draw_hline(10, 175, 102);

    // === 底部状态栏 ===
    draw_string(15, 185, "Status:OK");

    // 运行时间
    char time_str[16];
    if (uptime < 60) {
        snprintf(time_str, sizeof(time_str), "%lus", uptime);
    } else if (uptime < 3600) {
        snprintf(time_str, sizeof(time_str), "%lum", uptime / 60);
    } else {
        snprintf(time_str, sizeof(time_str), "%luh", uptime / 3600);
    }
    draw_string(15, 205, time_str);

    // === 装饰边框 ===
    draw_rect(5, 5, 112, 240);

    // 刷新屏幕
    eink_display_buffer(buffer);
    eink_refresh(EINK_REFRESH_FULL);
}

void app_main(void) {
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "═════════════════════════════════");
    ESP_LOGI(TAG, "  手写UI传感器显示程序");
    ESP_LOGI(TAG, "═════════════════════════════════");
    ESP_LOGI(TAG, "");

    // 1. 初始化I2C
    ESP_LOGI(TAG, "初始化I2C...");
    if (!i2c_master_init()) {
        ESP_LOGE(TAG, "I2C初始化失败！");
        return;
    }
    vTaskDelay(pdMS_TO_TICKS(100));

    // 2. 初始化墨水屏
    ESP_LOGI(TAG, "初始化墨水屏...");
    if (!eink_init()) {
        ESP_LOGE(TAG, "墨水屏初始化失败！");
        return;
    }
    ESP_LOGI(TAG, "墨水屏初始化成功: %dx%d", eink_get_width(), eink_get_height());

    // 3. 初始化传感器
    ESP_LOGI(TAG, "初始化AHT20...");
    if (!temp_sensor_init()) {
        ESP_LOGW(TAG, "AHT20初始化失败");
    }

    ESP_LOGI(TAG, "初始化AGS10...");
    if (!ags10_init()) {
        ESP_LOGW(TAG, "AGS10初始化失败");
    }

    vTaskDelay(pdMS_TO_TICKS(500));

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "═════════════════════════════════");
    ESP_LOGI(TAG, "  开始显示传感器数据");
    ESP_LOGI(TAG, "  每5秒刷新一次");
    ESP_LOGI(TAG, "═════════════════════════════════");
    ESP_LOGI(TAG, "");

    uint32_t update_count = 0;

    while (1) {
        update_count++;

        // 读取传感器数据
        float temperature = 0.0f;
        float humidity = 0.0f;
        uint32_t tvoc_ppb = 0;

        temp_sensor_get_temp_humidity(&temperature, &humidity);
        ags10_read_tvoc(&tvoc_ppb);

        // 计算运行时间
        uint32_t uptime_sec = xTaskGetTickCount() * portTICK_PERIOD_MS / 1000;

        // 绘制UI
        ESP_LOGI(TAG, "");
        ESP_LOGI(TAG, "═══ 更新 #%lu ═══", update_count);
        ESP_LOGI(TAG, "温度: %.1f°C", temperature);
        ESP_LOGI(TAG, "湿度: %.1f%%", humidity);
        ESP_LOGI(TAG, "TVOC: %lu ppb", tvoc_ppb);
        ESP_LOGI(TAG, "运行时间: %lu秒", uptime_sec);

        draw_ui(temperature, humidity, tvoc_ppb, uptime_sec);

        ESP_LOGI(TAG, "界面已刷新");

        // 等待5秒
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
