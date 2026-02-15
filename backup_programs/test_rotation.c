/**
 * @file test_rotation.c
 * @brief 测试屏幕旋转和大数字显示
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "ssd1680z_driver.h"
#include "I2C_driver.h"
#include "AHT20_temp_driver.h"
#include "ags10_driver.h"
#include <string.h>

static const char *TAG = "ROTATION_TEST";

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

// 绘制实心矩形
void fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    for (uint16_t dy = 0; dy < h; dy++) {
        draw_hline(x, y + dy, w);
    }
}

// 绘制放大版7段数码管（4倍大小）
void draw_digit_7seg_large(uint16_t x, uint16_t y, uint8_t digit) {
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

    // 放大4倍的段尺寸
    uint16_t seg_w = 40;  // 横段宽度
    uint16_t seg_h = 48;  // 竖段高度
    uint16_t seg_t = 8;   // 段厚度

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

void app_main(void) {
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "═══════════════════════════");
    ESP_LOGI(TAG, "  屏幕旋转测试");
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

    uint16_t width = eink_get_width();
    uint16_t height = eink_get_height();
    ESP_LOGI(TAG, "逻辑尺寸: %d x %d", width, height);

    // 3. 初始化传感器
    ESP_LOGI(TAG, "初始化传感器...");
    temp_sensor_init();
    ags10_init();
    vTaskDelay(pdMS_TO_TICKS(500));

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "═══════════════════════════");
    ESP_LOGI(TAG, "  开始测试显示");
    ESP_LOGI(TAG, "═══════════════════════════");
    ESP_LOGI(TAG, "");

    // 获取传感器数据
    float temperature = 0.0f;
    float humidity = 0.0f;
    temp_sensor_get_temp_humidity(&temperature, &humidity);

    ESP_LOGI(TAG, "温度: %.1f°C", temperature);
    ESP_LOGI(TAG, "湿度: %.1f%%", humidity);

    // 获取缓冲区
    uint8_t *buffer = eink_get_buffer();
    memset(buffer, 0xFF, (width * height) / 8);

    // 测试1: 绘制外框（验证旋转方向）
    ESP_LOGI(TAG, "绘制外框...");
    draw_hline(0, 0, width);           // 顶部
    draw_hline(0, height - 1, width);  // 底部
    draw_vline(0, 0, height);          // 左侧
    draw_vline(width - 1, 0, height);  // 右侧

    // 在四个角落绘制标记
    fill_rect(5, 5, 10, 10);                    // 左上角
    fill_rect(width - 15, 5, 10, 10);          // 右上角
    fill_rect(5, height - 15, 10, 10);         // 左下角
    fill_rect(width - 15, height - 15, 10, 10); // 右下角

    // 测试2: 绘制一个大数字（温度的个位数）
    uint8_t temp_digit = ((int)temperature) % 10;
    ESP_LOGI(TAG, "绘制大数字: %d", temp_digit);

    // 居中显示
    uint16_t digit_x = (width - 56) / 2;  // 数字宽度约56像素
    uint16_t digit_y = (height - 112) / 2; // 数字高度约112像素
    draw_digit_7seg_large(digit_x, digit_y, temp_digit);

    // 刷新屏幕
    ESP_LOGI(TAG, "刷新屏幕...");
    eink_display_buffer(buffer);
    eink_refresh(EINK_REFRESH_FULL);

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "═══════════════════════════");
    ESP_LOGI(TAG, "  测试完成！");
    ESP_LOGI(TAG, "  屏幕应该显示:");
    ESP_LOGI(TAG, "  - 外框和四角标记");
    ESP_LOGI(TAG, "  - 中央一个大数字: %d", temp_digit);
    ESP_LOGI(TAG, "═══════════════════════════");
    ESP_LOGI(TAG, "");

    // 保持运行
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
