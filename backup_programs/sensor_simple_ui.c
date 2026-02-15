/**
 * @file sensor_simple_ui.c
 * @brief 简化UI传感器显示 - 大数字显示
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

// 绘制实心矩形
void fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    for (uint16_t dy = 0; dy < h; dy++) {
        draw_hline(x, y + dy, w);
    }
}

// 绘制7段数码管样式的数字（大号，适合显示传感器数据）
void draw_digit_7seg(uint16_t x, uint16_t y, uint8_t digit) {
    // 7段显示定义：
    //  aaa
    // f   b
    //  ggg
    // e   c
    //  ddd

    // 每个段的开关状态 (a,b,c,d,e,f,g)
    const uint8_t segments[10] = {
        0b1111110, // 0
        0b0110000, // 1
        0b1101101, // 2
        0b1111001, // 3
        0b0110011, // 4
        0b1011011, // 5
        0b1011111, // 6
        0b1110000, // 7
        0b1111111, // 8
        0b1111011, // 9
    };

    if (digit > 9) return;

    uint8_t seg = segments[digit];

    // 段尺寸
    uint16_t seg_w = 10;  // 横段宽度
    uint16_t seg_h = 12;  // 竖段高度
    uint16_t seg_t = 2;   // 段厚度

    // a段 (顶部横)
    if (seg & 0b1000000) fill_rect(x + seg_t, y, seg_w, seg_t);

    // b段 (右上竖)
    if (seg & 0b0100000) fill_rect(x + seg_w + seg_t, y + seg_t, seg_t, seg_h);

    // c段 (右下竖)
    if (seg & 0b0010000) fill_rect(x + seg_w + seg_t, y + seg_h + seg_t * 2, seg_t, seg_h);

    // d段 (底部横)
    if (seg & 0b0001000) fill_rect(x + seg_t, y + seg_h * 2 + seg_t * 2, seg_w, seg_t);

    // e段 (左下竖)
    if (seg & 0b0000100) fill_rect(x, y + seg_h + seg_t * 2, seg_t, seg_h);

    // f段 (左上竖)
    if (seg & 0b0000010) fill_rect(x, y + seg_t, seg_t, seg_h);

    // g段 (中间横)
    if (seg & 0b0000001) fill_rect(x + seg_t, y + seg_h + seg_t, seg_w, seg_t);
}

// 绘制小数点
void draw_decimal_point(uint16_t x, uint16_t y) {
    fill_rect(x, y, 2, 2);
}

// 绘制2位数字（带小数点）
void draw_number_2digit(uint16_t x, uint16_t y, float value) {
    uint8_t tens = ((int)value / 10) % 10;
    uint8_t ones = ((int)value) % 10;
    uint8_t decimal = ((int)(value * 10)) % 10;

    draw_digit_7seg(x, y, tens);
    draw_digit_7seg(x + 18, y, ones);
    draw_decimal_point(x + 36, y + 24);
    draw_digit_7seg(x + 42, y, decimal);
}

// 绘制3位数字
void draw_number_3digit(uint16_t x, uint16_t y, uint32_t value) {
    uint8_t hundreds = (value / 100) % 10;
    uint8_t tens = (value / 10) % 10;
    uint8_t ones = value % 10;

    draw_digit_7seg(x, y, hundreds);
    draw_digit_7seg(x + 18, y, tens);
    draw_digit_7seg(x + 36, y, ones);
}

// 绘制完整UI (横向布局: 250宽 x 122高)
void draw_ui(float temp, float humidity, uint32_t tvoc) {
    uint8_t *buffer = eink_get_buffer();
    uint16_t width = eink_get_width();
    uint16_t height = eink_get_height();

    // 清空缓冲区
    memset(buffer, 0xFF, (width * height) / 8);

    // === 外框 ===
    draw_rect(2, 2, 246, 118);

    // === 温度区域 (左侧) ===
    // 标题装饰
    fill_rect(10, 10, 3, 3);
    fill_rect(10, 15, 3, 3);
    fill_rect(10, 20, 3, 3);

    // 温度数值 (大数字)
    draw_number_2digit(10, 35, temp);

    // 温度单位 °C
    draw_rect(60, 38, 6, 8);
    fill_rect(62, 40, 2, 2);

    // 竖分隔线
    draw_vline(82, 8, 106);

    // === 湿度区域 (中间) ===
    // 标题装饰
    fill_rect(90, 10, 3, 3);
    fill_rect(90, 15, 3, 3);
    fill_rect(90, 20, 3, 3);

    // 湿度数值
    draw_number_2digit(90, 35, humidity);

    // 湿度单位 %
    draw_rect(138, 38, 3, 3);
    draw_rect(143, 38, 3, 3);
    draw_hline(140, 40, 4);
    draw_rect(140, 43, 3, 3);
    draw_rect(145, 43, 3, 3);

    // 竖分隔线
    draw_vline(165, 8, 106);

    // === TVOC区域 (右侧) ===
    // 标题装饰
    fill_rect(173, 10, 3, 3);
    fill_rect(173, 15, 3, 3);
    fill_rect(173, 20, 3, 3);

    // TVOC数值
    draw_number_3digit(173, 35, tvoc);

    // 单位文字 (ppb用简单图形表示)
    fill_rect(227, 40, 2, 2);
    fill_rect(232, 40, 2, 2);
    fill_rect(237, 40, 2, 2);

    // === 底部状态指示 ===
    draw_hline(8, 75, 234);

    // 运行指示灯
    fill_rect(120, 85, 8, 8);
    fill_rect(118, 88, 12, 2);

    // 刷新屏幕
    eink_display_buffer(buffer);
    eink_refresh(EINK_REFRESH_FULL);
}

void app_main(void) {
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "═══════════════════════════");
    ESP_LOGI(TAG, "  简化UI传感器显示");
    ESP_LOGI(TAG, "═══════════════════════════");
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

    // 3. 初始化传感器
    ESP_LOGI(TAG, "初始化传感器...");
    temp_sensor_init();
    ags10_init();
    vTaskDelay(pdMS_TO_TICKS(500));

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "═══════════════════════════");
    ESP_LOGI(TAG, "  开始显示");
    ESP_LOGI(TAG, "  每5秒刷新");
    ESP_LOGI(TAG, "═══════════════════════════");
    ESP_LOGI(TAG, "");

    uint32_t count = 0;

    while (1) {
        count++;

        // 读取传感器
        float temperature = 0.0f;
        float humidity = 0.0f;
        uint32_t tvoc = 0;

        temp_sensor_get_temp_humidity(&temperature, &humidity);
        ags10_read_tvoc(&tvoc);

        ESP_LOGI(TAG, "");
        ESP_LOGI(TAG, "═══ #%lu ═══", count);
        ESP_LOGI(TAG, "温度: %.1f°C", temperature);
        ESP_LOGI(TAG, "湿度: %.1f%%", humidity);
        ESP_LOGI(TAG, "TVOC: %lu ppb", tvoc);

        // 绘制UI
        draw_ui(temperature, humidity, tvoc);

        ESP_LOGI(TAG, "界面已更新");

        // 等待5秒
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
