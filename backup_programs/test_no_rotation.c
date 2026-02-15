/**
 * @file test_no_rotation.c
 * @brief 测试不使用旋转，直接在物理坐标系绘制（竖屏122x250）
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "ssd1680z_driver.h"
#include <string.h>

static const char *TAG = "NO_ROTATE_TEST";

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

void app_main(void) {
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "═══════════════════════════");
    ESP_LOGI(TAG, "  不旋转测试 (竖屏)");
    ESP_LOGI(TAG, "═══════════════════════════");
    ESP_LOGI(TAG, "");

    // 初始化墨水屏（不使用旋转）
    ESP_LOGI(TAG, "初始化墨水屏...");

    if (!eink_init()) {
        ESP_LOGE(TAG, "墨水屏初始化失败！");
        return;
    }

    uint16_t width = eink_get_width();   // 应该是122
    uint16_t height = eink_get_height(); // 应该是250
    ESP_LOGI(TAG, "屏幕尺寸: %d x %d (竖屏)", width, height);

    // 获取缓冲区
    uint8_t *buffer = eink_get_buffer();
    memset(buffer, 0xFF, (width * height) / 8);

    ESP_LOGI(TAG, "绘制测试图形...");

    // 测试1: 外框
    draw_hline(0, 0, width);           // 顶部
    draw_hline(0, height - 1, width);  // 底部
    draw_vline(0, 0, height);          // 左侧
    draw_vline(width - 1, 0, height);  // 右侧

    // 测试2: 四个角的大方块
    fill_rect(5, 5, 20, 20);                    // 左上角
    fill_rect(width - 25, 5, 20, 20);          // 右上角
    fill_rect(5, height - 25, 20, 20);         // 左下角
    fill_rect(width - 25, height - 25, 20, 20); // 右下角

    // 测试3: 中央大十字
    uint16_t center_x = width / 2;
    uint16_t center_y = height / 2;
    draw_hline(center_x - 30, center_y, 60);  // 横线
    draw_vline(center_x, center_y - 30, 60);  // 竖线
    fill_rect(center_x - 10, center_y - 10, 20, 20); // 中心方块

    // 测试4: 顶部三个大方块（标记方向）
    fill_rect(10, 40, 30, 30);   // 左
    fill_rect(46, 40, 30, 30);   // 中
    fill_rect(82, 40, 30, 30);   // 右

    // 刷新屏幕
    ESP_LOGI(TAG, "刷新屏幕...");
    eink_display_buffer(buffer);
    eink_refresh(EINK_REFRESH_FULL);

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "═══════════════════════════");
    ESP_LOGI(TAG, "  测试完成！");
    ESP_LOGI(TAG, "  竖屏显示应该有:");
    ESP_LOGI(TAG, "  - 完整外框");
    ESP_LOGI(TAG, "  - 四角大方块");
    ESP_LOGI(TAG, "  - 中央十字");
    ESP_LOGI(TAG, "  - 顶部三个方块");
    ESP_LOGI(TAG, "═══════════════════════════");
    ESP_LOGI(TAG, "");

    // 保持运行
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
