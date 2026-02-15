/**
 * @file test_lvgl_simple.c
 * @brief 简单LVGL测试 - 验证文本显示
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lvgl.h"
#include "lvgl_eink_driver.h"
#include "squareline_ui.h"

static const char *TAG = "LVGL_TEST";

void app_main(void) {
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "═══════════════════════════════");
    ESP_LOGI(TAG, "   LVGL Simple Test");
    ESP_LOGI(TAG, "═══════════════════════════════");
    ESP_LOGI(TAG, "");

    // 1. 初始化LVGL和墨水屏
    ESP_LOGI(TAG, "Initializing LVGL...");
    if (!lvgl_eink_driver_init()) {
        ESP_LOGE(TAG, "LVGL initialization failed!");
        return;
    }
    ESP_LOGI(TAG, "LVGL initialized");
    vTaskDelay(pdMS_TO_TICKS(500));

    // 2. 创建简单的UI
    ESP_LOGI(TAG, "Creating test UI...");
    if (lvgl_lock()) {
        // 创建屏幕
        lv_obj_t *screen = lv_obj_create(NULL);
        lv_scr_load(screen);
        lv_obj_set_style_bg_color(screen, lv_color_white(), 0);

        // 创建大号文本标签 - 顶部
        lv_obj_t *label1 = lv_label_create(screen);
        lv_label_set_text(label1, "LVGL Test");
        lv_obj_set_style_text_font(label1, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(label1, lv_color_black(), 0);
        lv_obj_align(label1, LV_ALIGN_TOP_MID, 0, 20);

        // 创建中号文本标签 - 中间
        lv_obj_t *label2 = lv_label_create(screen);
        lv_label_set_text(label2, "122x250");
        lv_obj_set_style_text_font(label2, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(label2, lv_color_black(), 0);
        lv_obj_align(label2, LV_ALIGN_CENTER, 0, 0);

        // 创建小号文本标签 - 底部
        lv_obj_t *label3 = lv_label_create(screen);
        lv_label_set_text(label3, "E-ink Display");
        lv_obj_set_style_text_font(label3, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(label3, lv_color_black(), 0);
        lv_obj_align(label3, LV_ALIGN_BOTTOM_MID, 0, -20);

        lvgl_unlock();
    }

    // 等待LVGL渲染
    vTaskDelay(pdMS_TO_TICKS(200));

    // 3. 刷新屏幕
    ESP_LOGI(TAG, "Displaying UI...");
    if (lvgl_lock()) {
        lvgl_eink_set_refresh_mode(true);  // 全刷新
        lvgl_eink_force_refresh();
        lvgl_unlock();
    }

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "═══════════════════════════════");
    ESP_LOGI(TAG, "   Test Complete!");
    ESP_LOGI(TAG, "   屏幕应该显示:");
    ESP_LOGI(TAG, "   - 顶部: LVGL Test");
    ESP_LOGI(TAG, "   - 中间: 122x250");
    ESP_LOGI(TAG, "   - 底部: E-ink Display");
    ESP_LOGI(TAG, "═══════════════════════════════");
    ESP_LOGI(TAG, "");

    // 保持运行
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
