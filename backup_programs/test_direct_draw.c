/**
 * @file test_direct_draw.c
 * @brief 直接绘制测试 - 跳过LVGL直接写墨水屏
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "ssd1680z_driver.h"
#include <string.h>

static const char *TAG = "DIRECT_TEST";

void app_main(void) {
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "═══════════════════════════════");
    ESP_LOGI(TAG, "   Direct Draw Test");
    ESP_LOGI(TAG, "═══════════════════════════════");
    ESP_LOGI(TAG, "");

    // 初始化墨水屏
    ESP_LOGI(TAG, "Initializing e-ink...");
    if (!eink_init()) {
        ESP_LOGE(TAG, "E-ink init failed!");
        return;
    }
    ESP_LOGI(TAG, "E-ink initialized: %dx%d", eink_get_width(), eink_get_height());

    // 获取缓冲区
    uint8_t *buffer = eink_get_buffer();
    uint16_t width = eink_get_width();
    uint16_t height = eink_get_height();

    // 测试1: 清屏（全白）
    ESP_LOGI(TAG, "Test 1: Clear screen (white)");
    eink_clear();
    eink_refresh(EINK_REFRESH_FULL);
    vTaskDelay(pdMS_TO_TICKS(2000));

    // 测试2: 绘制黑色矩形框
    ESP_LOGI(TAG, "Test 2: Draw black rectangle");

    // 顶部横线 (y=10, x=10-111)
    for (uint16_t x = 10; x < 112; x++) {
        eink_set_pixel(x, 10, 0);  // 黑色
    }

    // 底部横线 (y=240, x=10-111)
    for (uint16_t x = 10; x < 112; x++) {
        eink_set_pixel(x, 240, 0);  // 黑色
    }

    // 左侧竖线 (x=10, y=10-240)
    for (uint16_t y = 10; y <= 240; y++) {
        eink_set_pixel(10, y, 0);  // 黑色
    }

    // 右侧竖线 (x=111, y=10-240)
    for (uint16_t y = 10; y <= 240; y++) {
        eink_set_pixel(111, y, 0);  // 黑色
    }

    // 绘制对角线
    for (uint16_t i = 0; i < 100; i++) {
        eink_set_pixel(10 + i, 10 + (i * 230 / 100), 0);  // 从左上到右下
    }

    // 刷新显示
    eink_display_buffer(buffer);
    eink_refresh(EINK_REFRESH_FULL);

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "═══════════════════════════════");
    ESP_LOGI(TAG, "   Test Complete!");
    ESP_LOGI(TAG, "   屏幕应该显示:");
    ESP_LOGI(TAG, "   - 黑色矩形框");
    ESP_LOGI(TAG, "   - 对角线");
    ESP_LOGI(TAG, "═══════════════════════════════");
    ESP_LOGI(TAG, "");

    // 保持运行
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
