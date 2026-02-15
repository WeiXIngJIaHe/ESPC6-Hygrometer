/**
 * @file test_buffer_direct.c
 * @brief 直接操作缓冲区字节，验证正确的缓冲区格式
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "ssd1680z_driver.h"
#include <string.h>

static const char *TAG = "BUF_DIRECT_TEST";

#define PHYS_WIDTH  122
#define PHYS_HEIGHT 250
#define BUF_SIZE    ((PHYS_WIDTH * PHYS_HEIGHT) / 8)

void app_main(void) {
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "═══════════════════════════");
    ESP_LOGI(TAG, "  缓冲区直接操作测试");
    ESP_LOGI(TAG, "═══════════════════════════");
    ESP_LOGI(TAG, "");

    // 初始化墨水屏
    if (!eink_init()) {
        ESP_LOGE(TAG, "墨水屏初始化失败！");
        return;
    }

    uint8_t *buffer = eink_get_buffer();
    ESP_LOGI(TAG, "缓冲区大小: %d bytes (122x250/8)", BUF_SIZE);

    // 测试1: 顶部10行全黑
    ESP_LOGI(TAG, "测试1: 顶部10行全黑 (等待3秒)...");
    memset(buffer, 0xFF, BUF_SIZE);  // 先全白

    // 122像素/行 = 122/8 = 15.25,向上取整16字节/行
    size_t bytes_per_row = (PHYS_WIDTH + 7) / 8;  // 16字节
    ESP_LOGI(TAG, "每行字节数: %d", bytes_per_row);

    // 前10行设为黑色
    for (int row = 0; row < 10; row++) {
        size_t row_offset = row * bytes_per_row;
        memset(buffer + row_offset, 0x00, bytes_per_row);
    }

    eink_display_buffer(buffer);
    eink_refresh(EINK_REFRESH_FULL);
    vTaskDelay(pdMS_TO_TICKS(3000));

    // 测试2: 左半边黑色
    ESP_LOGI(TAG, "测试2: 左半边黑色 (等待3秒)...");
    memset(buffer, 0xFF, BUF_SIZE);  // 先全白

    // 每行的前8字节（64像素）设为黑色
    for (int row = 0; row < PHYS_HEIGHT; row++) {
        size_t row_offset = row * bytes_per_row;
        memset(buffer + row_offset, 0x00, 8);  // 前8字节
    }

    eink_display_buffer(buffer);
    eink_refresh(EINK_REFRESH_FULL);
    vTaskDelay(pdMS_TO_TICKS(3000));

    // 测试3: 竖条纹（8像素间隔）
    ESP_LOGI(TAG, "测试3: 竖条纹8像素间隔 (等待3秒)...");
    memset(buffer, 0xFF, BUF_SIZE);

    // 每个字节交替黑白 0x00, 0xFF, 0x00, 0xFF...
    for (size_t i = 0; i < BUF_SIZE; i++) {
        buffer[i] = (i % 2) ? 0xFF : 0x00;
    }

    eink_display_buffer(buffer);
    eink_refresh(EINK_REFRESH_FULL);
    vTaskDelay(pdMS_TO_TICKS(3000));

    // 测试4: 横条纹（10行间隔）
    ESP_LOGI(TAG, "测试4: 横条纹10行间隔 (最终)...");
    memset(buffer, 0xFF, BUF_SIZE);

    for (int row = 0; row < PHYS_HEIGHT; row++) {
        size_t row_offset = row * bytes_per_row;
        uint8_t color = ((row / 10) % 2) ? 0xFF : 0x00;
        memset(buffer + row_offset, color, bytes_per_row);
    }

    eink_display_buffer(buffer);
    eink_refresh(EINK_REFRESH_FULL);

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "═══════════════════════════");
    ESP_LOGI(TAG, "  测试完成！");
    ESP_LOGI(TAG, "  观察显示模式：");
    ESP_LOGI(TAG, "  1. 顶部10行黑色");
    ESP_LOGI(TAG, "  2. 左半边黑色");
    ESP_LOGI(TAG, "  3. 竖条纹(8像素)");
    ESP_LOGI(TAG, "  4. 横条纹(10行) 最终");
    ESP_LOGI(TAG, "═══════════════════════════");
    ESP_LOGI(TAG, "");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
