/**
 * @file test_eink_simple.c
 * @brief 简单的墨水屏测试程序 - 不使用LVGL
 * @note 直接测试SSD1680Z驱动
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "ssd1680z_driver.h"
#include <string.h>

static const char *TAG = "EINK_TEST";

void app_main(void) {
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "═════════════════════════════════════════════");
    ESP_LOGI(TAG, "       墨水屏简单测试程序");
    ESP_LOGI(TAG, "═════════════════════════════════════════════");
    ESP_LOGI(TAG, "屏幕: SSD1680Z (122x250)");
    ESP_LOGI(TAG, "测试: 全黑 → 全白 → 斑马纹");
    ESP_LOGI(TAG, "═════════════════════════════════════════════");
    ESP_LOGI(TAG, "");

    // 初始化墨水屏
    ESP_LOGI(TAG, "初始化墨水屏...");
    if (!eink_init()) {
        ESP_LOGE(TAG, "墨水屏初始化失败！");
        return;
    }
    ESP_LOGI(TAG, "墨水屏初始化成功！");
    vTaskDelay(pdMS_TO_TICKS(1000));

    // 获取缓冲区指针
    uint8_t *buffer = eink_get_buffer();
    uint16_t width = eink_get_width();
    uint16_t height = eink_get_height();

    ESP_LOGI(TAG, "屏幕尺寸: %dx%d", width, height);
    ESP_LOGI(TAG, "缓冲区大小: %d 字节", (width * height) / 8);
    ESP_LOGI(TAG, "");

    // 测试1: 全白屏幕
    ESP_LOGI(TAG, "测试1: 全白屏幕");
    ESP_LOGI(TAG, "清空屏幕...");
    eink_clear();
    ESP_LOGI(TAG, "刷新屏幕...");
    eink_refresh(EINK_REFRESH_FULL);
    ESP_LOGI(TAG, "完成！等待5秒...");
    vTaskDelay(pdMS_TO_TICKS(5000));

    // 测试2: 全黑屏幕
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "测试2: 全黑屏幕");
    ESP_LOGI(TAG, "填充黑色...");
    memset(buffer, 0x00, (width * height) / 8);
    eink_display_buffer(buffer);
    ESP_LOGI(TAG, "刷新屏幕...");
    eink_refresh(EINK_REFRESH_FULL);
    ESP_LOGI(TAG, "完成！等待5秒...");
    vTaskDelay(pdMS_TO_TICKS(5000));

    // 测试3: 斑马纹（横向条纹）
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "测试3: 横向斑马纹");
    ESP_LOGI(TAG, "生成斑马纹图案...");
    for (uint16_t y = 0; y < height; y++) {
        for (uint16_t x = 0; x < width; x++) {
            // 每10行一个条纹
            uint8_t color = (y / 10) % 2;
            eink_set_pixel(x, y, color);
        }
    }
    ESP_LOGI(TAG, "写入缓冲区...");
    eink_display_buffer(buffer);
    ESP_LOGI(TAG, "刷新屏幕...");
    eink_refresh(EINK_REFRESH_FULL);
    ESP_LOGI(TAG, "完成！等待5秒...");
    vTaskDelay(pdMS_TO_TICKS(5000));

    // 测试4: 竖向条纹
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "测试4: 竖向斑马纹");
    ESP_LOGI(TAG, "生成斑马纹图案...");
    for (uint16_t y = 0; y < height; y++) {
        for (uint16_t x = 0; x < width; x++) {
            // 每10列一个条纹
            uint8_t color = (x / 10) % 2;
            eink_set_pixel(x, y, color);
        }
    }
    ESP_LOGI(TAG, "写入缓冲区...");
    eink_display_buffer(buffer);
    ESP_LOGI(TAG, "刷新屏幕...");
    eink_refresh(EINK_REFRESH_FULL);
    ESP_LOGI(TAG, "完成！");
    vTaskDelay(pdMS_TO_TICKS(5000));

    // 测试5: 棋盘格
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "测试5: 棋盘格图案");
    ESP_LOGI(TAG, "生成棋盘格...");
    for (uint16_t y = 0; y < height; y++) {
        for (uint16_t x = 0; x < width; x++) {
            uint8_t color = ((x / 10) + (y / 10)) % 2;
            eink_set_pixel(x, y, color);
        }
    }
    ESP_LOGI(TAG, "写入缓冲区...");
    eink_display_buffer(buffer);
    ESP_LOGI(TAG, "刷新屏幕...");
    eink_refresh(EINK_REFRESH_FULL);
    ESP_LOGI(TAG, "完成！");

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "═════════════════════════════════════════════");
    ESP_LOGI(TAG, "       所有测试完成！");
    ESP_LOGI(TAG, "═════════════════════════════════════════════");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "如果屏幕有显示，说明驱动工作正常");
    ESP_LOGI(TAG, "如果屏幕无显示，请检查:");
    ESP_LOGI(TAG, "  1. SPI引脚连接 (MOSI=%d, SCLK=%d, CS=%d)",
             EINK_MOSI_PIN, EINK_SCLK_PIN, EINK_CS_PIN);
    ESP_LOGI(TAG, "  2. 控制引脚连接 (DC=%d, RST=%d, BUSY=%d)",
             EINK_DC_PIN, EINK_RST_PIN, EINK_BUSY_PIN);
    ESP_LOGI(TAG, "  3. 屏幕供电 (3.3V)");
    ESP_LOGI(TAG, "");

    // 进入睡眠模式
    ESP_LOGI(TAG, "进入深度睡眠模式...");
    eink_sleep();

    ESP_LOGI(TAG, "程序结束");
}
