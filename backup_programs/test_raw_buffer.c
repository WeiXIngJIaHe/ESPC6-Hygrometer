/**
 * @file test_raw_buffer.c
 * @brief 直接操作缓冲区，完全绕过eink_set_pixel
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "ssd1680z_driver.h"
#include <string.h>

static const char *TAG = "RAW_BUFFER_TEST";

// 物理尺寸
#define PHYS_WIDTH  122
#define PHYS_HEIGHT 250

// 直接设置缓冲区的像素（完全不经过eink_set_pixel）
void raw_set_pixel(uint8_t *buffer, uint16_t x, uint16_t y, uint8_t color) {
    if (x >= PHYS_WIDTH || y >= PHYS_HEIGHT) return;

    uint32_t addr = (y * PHYS_WIDTH + x) / 8;
    uint8_t bit = 7 - (x % 8);

    if (color == 0) {
        // 黑色
        buffer[addr] &= ~(1 << bit);
    } else {
        // 白色
        buffer[addr] |= (1 << bit);
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "═══════════════════════════");
    ESP_LOGI(TAG, "  原始缓冲区测试");
    ESP_LOGI(TAG, "═══════════════════════════");
    ESP_LOGI(TAG, "");

    // 初始化墨水屏
    ESP_LOGI(TAG, "初始化墨水屏...");
    if (!eink_init()) {
        ESP_LOGE(TAG, "墨水屏初始化失败！");
        return;
    }

    ESP_LOGI(TAG, "屏幕尺寸: %d x %d", eink_get_width(), eink_get_height());

    // 获取缓冲区
    uint8_t *buffer = eink_get_buffer();

    // 清空缓冲区（全白）
    memset(buffer, 0xFF, (PHYS_WIDTH * PHYS_HEIGHT) / 8);

    ESP_LOGI(TAG, "直接绘制到缓冲区...");

    // 测试1: 完整外框
    ESP_LOGI(TAG, "绘制外框...");
    for (uint16_t x = 0; x < PHYS_WIDTH; x++) {
        raw_set_pixel(buffer, x, 0, 0);                    // 顶部
        raw_set_pixel(buffer, x, PHYS_HEIGHT - 1, 0);     // 底部
    }
    for (uint16_t y = 0; y < PHYS_HEIGHT; y++) {
        raw_set_pixel(buffer, 0, y, 0);                    // 左侧
        raw_set_pixel(buffer, PHYS_WIDTH - 1, y, 0);      // 右侧
    }

    // 测试2: 四个角的大方块（30x30）
    ESP_LOGI(TAG, "绘制四角方块...");
    for (uint16_t dy = 0; dy < 30; dy++) {
        for (uint16_t dx = 0; dx < 30; dx++) {
            raw_set_pixel(buffer, 10 + dx, 10 + dy, 0);                          // 左上
            raw_set_pixel(buffer, PHYS_WIDTH - 40 + dx, 10 + dy, 0);            // 右上
            raw_set_pixel(buffer, 10 + dx, PHYS_HEIGHT - 40 + dy, 0);           // 左下
            raw_set_pixel(buffer, PHYS_WIDTH - 40 + dx, PHYS_HEIGHT - 40 + dy, 0); // 右下
        }
    }

    // 测试3: 中央十字
    ESP_LOGI(TAG, "绘制中央十字...");
    uint16_t center_x = PHYS_WIDTH / 2;
    uint16_t center_y = PHYS_HEIGHT / 2;

    // 横线
    for (uint16_t x = center_x - 50; x < center_x + 50; x++) {
        for (uint16_t t = 0; t < 5; t++) {  // 线宽5
            raw_set_pixel(buffer, x, center_y + t - 2, 0);
        }
    }

    // 竖线
    for (uint16_t y = center_y - 50; y < center_y + 50; y++) {
        for (uint16_t t = 0; t < 5; t++) {  // 线宽5
            raw_set_pixel(buffer, center_x + t - 2, y, 0);
        }
    }

    // 测试4: 顶部三个大方块标记
    ESP_LOGI(TAG, "绘制顶部标记...");
    for (uint16_t dy = 0; dy < 40; dy++) {
        for (uint16_t dx = 0; dx < 35; dx++) {
            raw_set_pixel(buffer, 5 + dx, 50 + dy, 0);   // 左
            raw_set_pixel(buffer, 43 + dx, 50 + dy, 0);  // 中
            raw_set_pixel(buffer, 82 + dx, 50 + dy, 0);  // 右
        }
    }

    // 刷新屏幕
    ESP_LOGI(TAG, "刷新屏幕...");
    eink_display_buffer(buffer);
    eink_refresh(EINK_REFRESH_FULL);

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "═══════════════════════════");
    ESP_LOGI(TAG, "  测试完成！");
    ESP_LOGI(TAG, "  竖屏(122x250)应显示:");
    ESP_LOGI(TAG, "  - 完整外框");
    ESP_LOGI(TAG, "  - 四角大方块(30x30)");
    ESP_LOGI(TAG, "  - 中央粗十字");
    ESP_LOGI(TAG, "  - 顶部3个方块(35x40)");
    ESP_LOGI(TAG, "═══════════════════════════");
    ESP_LOGI(TAG, "");

    // 保持运行
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
