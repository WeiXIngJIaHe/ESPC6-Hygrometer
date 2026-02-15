/**
 * @file test_coordinate.c
 * @brief 测试坐标系统 - 绘制简单条纹模式来确定正确的映射
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "ssd1680z_driver.h"
#include <string.h>

static const char *TAG = "COORD_TEST";

#define PHYS_WIDTH  122
#define PHYS_HEIGHT 250

void app_main(void) {
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "═══════════════════════════");
    ESP_LOGI(TAG, "  坐标系统测试");
    ESP_LOGI(TAG, "═══════════════════════════");
    ESP_LOGI(TAG, "");

    // 初始化墨水屏
    if (!eink_init()) {
        ESP_LOGE(TAG, "墨水屏初始化失败！");
        return;
    }

    ESP_LOGI(TAG, "屏幕尺寸: %dx%d", eink_get_width(), eink_get_height());

    // 获取缓冲区
    uint8_t *buffer = eink_get_buffer();
    size_t buffer_size = (PHYS_WIDTH * PHYS_HEIGHT) / 8;

    ESP_LOGI(TAG, "缓冲区大小: %d bytes", buffer_size);

    // 测试1: 全黑屏
    ESP_LOGI(TAG, "测试1: 全黑屏 (等待5秒)...");
    memset(buffer, 0x00, buffer_size);  // 全黑
    eink_display_buffer(buffer);
    eink_refresh(EINK_REFRESH_FULL);
    vTaskDelay(pdMS_TO_TICKS(5000));

    // 测试2: 全白屏
    ESP_LOGI(TAG, "测试2: 全白屏 (等待5秒)...");
    memset(buffer, 0xFF, buffer_size);  // 全白
    eink_display_buffer(buffer);
    eink_refresh(EINK_REFRESH_FULL);
    vTaskDelay(pdMS_TO_TICKS(5000));

    // 测试3: 简单的黑白条纹（字节级别）
    ESP_LOGI(TAG, "测试3: 字节级条纹 (等待5秒)...");
    for (size_t i = 0; i < buffer_size; i++) {
        // 每个字节交替 0x00(黑) 和 0xFF(白)
        buffer[i] = (i % 2) ? 0xFF : 0x00;
    }
    eink_display_buffer(buffer);
    eink_refresh(EINK_REFRESH_FULL);
    vTaskDelay(pdMS_TO_TICKS(5000));

    // 测试4: 每8字节一个条纹
    ESP_LOGI(TAG, "测试4: 8字节条纹 (等待5秒)...");
    for (size_t i = 0; i < buffer_size; i++) {
        buffer[i] = ((i / 8) % 2) ? 0xFF : 0x00;
    }
    eink_display_buffer(buffer);
    eink_refresh(EINK_REFRESH_FULL);
    vTaskDelay(pdMS_TO_TICKS(5000));

    // 测试5: 按行条纹（假设一行是122像素 = 122/8 ≈ 16字节）
    ESP_LOGI(TAG, "测试5: 按行条纹 (每行%d字节, 等待5秒)...", PHYS_WIDTH / 8);
    size_t bytes_per_row = PHYS_WIDTH / 8;
    for (size_t i = 0; i < buffer_size; i++) {
        size_t row = i / bytes_per_row;
        buffer[i] = (row % 10 < 5) ? 0xFF : 0x00;  // 每10行切换
    }
    eink_display_buffer(buffer);
    eink_refresh(EINK_REFRESH_FULL);
    vTaskDelay(pdMS_TO_TICKS(5000));

    // 测试6: 左半边黑，右半边白
    ESP_LOGI(TAG, "测试6: 左黑右白 (等待5秒)...");
    memset(buffer, 0xFF, buffer_size);  // 先全白
    // 左半边设为黑色（前半段字节）
    memset(buffer, 0x00, buffer_size / 2);
    eink_display_buffer(buffer);
    eink_refresh(EINK_REFRESH_FULL);
    vTaskDelay(pdMS_TO_TICKS(5000));

    // 测试7: 棋盘格（每个字节代表8像素）
    ESP_LOGI(TAG, "测试7: 棋盘格 (等待5秒)...");
    for (size_t i = 0; i < buffer_size; i++) {
        // 每个字节内部棋盘: 0xAA = 10101010, 0x55 = 01010101
        size_t row_byte = i / bytes_per_row;
        size_t col_byte = i % bytes_per_row;
        buffer[i] = ((row_byte + col_byte) % 2) ? 0xAA : 0x55;
    }
    eink_display_buffer(buffer);
    eink_refresh(EINK_REFRESH_FULL);

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "═══════════════════════════");
    ESP_LOGI(TAG, "  测试完成！");
    ESP_LOGI(TAG, "  观察屏幕显示的模式:");
    ESP_LOGI(TAG, "  1. 全黑");
    ESP_LOGI(TAG, "  2. 全白");
    ESP_LOGI(TAG, "  3. 字节交替条纹");
    ESP_LOGI(TAG, "  4. 8字节条纹");
    ESP_LOGI(TAG, "  5. 行条纹(每10行)");
    ESP_LOGI(TAG, "  6. 左黑右白");
    ESP_LOGI(TAG, "  7. 棋盘格(最终)");
    ESP_LOGI(TAG, "═══════════════════════════");
    ESP_LOGI(TAG, "");

    // 保持运行
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
