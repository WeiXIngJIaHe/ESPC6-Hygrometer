/**
 * @file main_sensor_display.c
 * @brief 传感器数据屏幕显示程序 - AHT20温湿度 + AGS10空气质量
 * @note 在SSD1680Z e-ink屏幕 (122x250) 上显示传感器数据
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lvgl.h"
#include "lvgl_eink_driver.h"
#include "squareline_ui.h"
#include "I2C_driver.h"
#include "AHT20_temp_driver.h"
#include "ags10_driver.h"
#include <stdio.h>

static const char *TAG = "SENSOR_DISPLAY";

// ============================================================================
// LVGL UI组件
// ============================================================================

static lv_obj_t *screen;
static lv_obj_t *title_label;
static lv_obj_t *temp_label;
static lv_obj_t *humidity_label;
static lv_obj_t *tvoc_label;
static lv_obj_t *status_label;
static lv_obj_t *time_label;
static lv_obj_t *progress_dots[5];  // 5个进度点

// 进度点状态
static uint8_t current_dot = 0;  // 当前显示到第几个点 (0-4)

// ============================================================================
// UI初始化
// ============================================================================

void create_sensor_ui(void) {
    // 创建主屏幕
    screen = lv_obj_create(NULL);
    lv_scr_load(screen);
    lv_obj_set_style_bg_color(screen, lv_color_white(), 0);

    // 标题
    title_label = lv_label_create(screen);
    lv_label_set_text(title_label, "ESP32-C6 Sensors");
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(title_label, lv_color_black(), 0);  // 黑色文本
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 10);

    // 温度显示
    temp_label = lv_label_create(screen);
    lv_label_set_text(temp_label, "Temp: -- C");
    lv_obj_set_style_text_font(temp_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(temp_label, lv_color_black(), 0);
    lv_obj_align(temp_label, LV_ALIGN_TOP_LEFT, 10, 40);

    // 湿度显示
    humidity_label = lv_label_create(screen);
    lv_label_set_text(humidity_label, "Humidity: -- %");
    lv_obj_set_style_text_font(humidity_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(humidity_label, lv_color_black(), 0);
    lv_obj_align(humidity_label, LV_ALIGN_TOP_LEFT, 10, 65);

    // TVOC显示
    tvoc_label = lv_label_create(screen);
    lv_label_set_text(tvoc_label, "TVOC: -- ppb");
    lv_obj_set_style_text_font(tvoc_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(tvoc_label, lv_color_black(), 0);
    lv_obj_align(tvoc_label, LV_ALIGN_TOP_LEFT, 10, 90);

    // 状态显示
    status_label = lv_label_create(screen);
    lv_label_set_text(status_label, "Status: Initializing...");
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(status_label, lv_color_black(), 0);
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_LEFT, 10, -30);

    // 运行时间显示
    time_label = lv_label_create(screen);
    lv_label_set_text(time_label, "Uptime: 0s");
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(time_label, lv_color_black(), 0);
    lv_obj_align(time_label, LV_ALIGN_BOTTOM_LEFT, 10, -10);

    // 创建进度点（5个点，用于显示刷新倒计时）
    for (int i = 0; i < 5; i++) {
        progress_dots[i] = lv_label_create(screen);
        lv_label_set_text(progress_dots[i], "·");  // 空心点
        lv_obj_set_style_text_font(progress_dots[i], &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(progress_dots[i], lv_color_black(), 0);
        lv_obj_align(progress_dots[i], LV_ALIGN_BOTTOM_MID, -40 + i * 20, -10);
    }

    ESP_LOGI(TAG, "UI created successfully");
}

// ============================================================================
// 进度点更新（局部刷新）
// ============================================================================

void update_progress_dot(void) {
    if (lvgl_lock()) {
        // 更新当前点为实心点
        if (current_dot < 5) {
            lv_label_set_text(progress_dots[current_dot], "●");  // 实心点
        }
        lvgl_unlock();
    }

    // 等待LVGL渲染
    vTaskDelay(pdMS_TO_TICKS(50));

    // 局部刷新屏幕
    if (lvgl_lock()) {
        lvgl_eink_set_refresh_mode(false);  // false = 局部刷新
        lvgl_eink_force_refresh();
        lvgl_unlock();
    }

    ESP_LOGI(TAG, "Progress dot %d updated (partial refresh)", current_dot + 1);
    current_dot++;
}

// ============================================================================
// 重置进度点
// ============================================================================

void reset_progress_dots(void) {
    if (lvgl_lock()) {
        // 重置所有点为空心点
        for (int i = 0; i < 5; i++) {
            lv_label_set_text(progress_dots[i], "·");  // 空心点
        }
        lvgl_unlock();
    }

    // 等待LVGL渲染
    vTaskDelay(pdMS_TO_TICKS(50));

    // 局部刷新屏幕
    if (lvgl_lock()) {
        lvgl_eink_set_refresh_mode(false);  // false = 局部刷新
        lvgl_eink_force_refresh();
        lvgl_unlock();
    }

    current_dot = 0;
    ESP_LOGI(TAG, "Progress dots reset (partial refresh)");
}

// ============================================================================
// 传感器数据更新
// ============================================================================

void update_sensor_display(void) {
    static uint32_t update_count = 0;
    update_count++;

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "======= Sensor Update #%lu =======", update_count);

    // 读取AHT20温湿度数据
    float temperature = 0.0f;
    float humidity = 0.0f;
    bool aht20_ok = temp_sensor_get_temp_humidity(&temperature, &humidity);

    // 读取AGS10 TVOC数据
    uint32_t tvoc_ppb = 0;
    bool ags10_ok = ags10_read_tvoc(&tvoc_ppb);

    // 获取LVGL锁
    if (lvgl_lock()) {
        // 更新温度
        if (aht20_ok) {
            char temp_buf[32];
            snprintf(temp_buf, sizeof(temp_buf), "Temp: %.1f C", temperature);
            lv_label_set_text(temp_label, temp_buf);

            // 更新湿度
            char humi_buf[32];
            snprintf(humi_buf, sizeof(humi_buf), "Humidity: %.1f %%", humidity);
            lv_label_set_text(humidity_label, humi_buf);
        } else {
            lv_label_set_text(temp_label, "Temp: Error");
            lv_label_set_text(humidity_label, "Humidity: Error");
        }

        // 更新TVOC
        if (ags10_ok) {
            char tvoc_buf[32];
            snprintf(tvoc_buf, sizeof(tvoc_buf), "TVOC: %lu ppb", tvoc_ppb);
            lv_label_set_text(tvoc_label, tvoc_buf);
        } else {
            lv_label_set_text(tvoc_label, "TVOC: Error");
        }

        // 更新状态
        if (aht20_ok && ags10_ok) {
            lv_label_set_text(status_label, "Status: All OK");
        } else if (aht20_ok || ags10_ok) {
            lv_label_set_text(status_label, "Status: Partial");
        } else {
            lv_label_set_text(status_label, "Status: Error");
        }

        // 更新运行时间
        uint32_t uptime_sec = xTaskGetTickCount() * portTICK_PERIOD_MS / 1000;
        char time_buf[32];
        snprintf(time_buf, sizeof(time_buf), "Uptime: %lus", uptime_sec);
        lv_label_set_text(time_label, time_buf);

        lvgl_unlock();
    }

    // 等待LVGL渲染
    vTaskDelay(pdMS_TO_TICKS(100));

    // 全刷新屏幕
    if (lvgl_lock()) {
        lvgl_eink_set_refresh_mode(true);  // true = 全刷新
        lvgl_eink_force_refresh();
        lvgl_unlock();
    }

    ESP_LOGI(TAG, "Temp: %.1f C, Humidity: %.1f%%, TVOC: %lu ppb",
             temperature, humidity, tvoc_ppb);
    ESP_LOGI(TAG, "Display updated (FULL REFRESH)");
    ESP_LOGI(TAG, "===================================");
}

// ============================================================================
// 传感器任务
// ============================================================================

void sensor_task(void *arg) {
    ESP_LOGI(TAG, "Sensor task started");

    // 初始化传感器
    ESP_LOGI(TAG, "Initializing AHT20...");
    if (temp_sensor_init()) {
        ESP_LOGI(TAG, "AHT20 initialized successfully");
    } else {
        ESP_LOGE(TAG, "AHT20 initialization failed");
    }

    vTaskDelay(pdMS_TO_TICKS(500));

    ESP_LOGI(TAG, "Initializing AGS10...");
    if (ags10_init()) {
        ESP_LOGI(TAG, "AGS10 initialized successfully");
    } else {
        ESP_LOGE(TAG, "AGS10 initialization failed");
    }

    vTaskDelay(pdMS_TO_TICKS(1000));

    // 首次立即更新传感器数据
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Performing initial sensor data update...");
    update_sensor_display();
    vTaskDelay(pdMS_TO_TICKS(1000));

    // 主循环：每1秒更新进度点，5秒刷新传感器数据
    while (1) {
        // 重置进度点
        reset_progress_dots();

        // 5次循环，每次1秒
        for (int i = 0; i < 5; i++) {
            // 等待1秒
            vTaskDelay(pdMS_TO_TICKS(1000));

            // 更新进度点（局部刷新，快速）
            update_progress_dot();
        }

        // 5个点满了，刷新传感器数据（全刷新）
        update_sensor_display();
    }
}

// ============================================================================
// 主程序
// ============================================================================

void app_main(void) {
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "═════════════════════════════════════════════");
    ESP_LOGI(TAG, "    ESP32-C6 传感器数据屏幕显示程序");
    ESP_LOGI(TAG, "═════════════════════════════════════════════");
    ESP_LOGI(TAG, "显示内容:");
    ESP_LOGI(TAG, "  • AHT20  - 温度和湿度");
    ESP_LOGI(TAG, "  • AGS10  - TVOC空气质量");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "刷新策略:");
    ESP_LOGI(TAG, "  • 进度点 - 每1秒显示1个点 (局部刷新)");
    ESP_LOGI(TAG, "  • 传感器 - 5个点满后更新数据 (全刷新)");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "硬件配置:");
    ESP_LOGI(TAG, "  • 屏幕: SSD1680Z (122x250)");
    ESP_LOGI(TAG, "  • I2C:  SDA=GPIO6, SCL=GPIO7");
    ESP_LOGI(TAG, "═════════════════════════════════════════════");

    // 1. 初始化I2C总线
    ESP_LOGI(TAG, "Initializing I2C bus...");
    if (!i2c_master_init()) {
        ESP_LOGE(TAG, "I2C initialization failed!");
        return;
    }
    ESP_LOGI(TAG, "I2C initialized (SDA=GPIO6, SCL=GPIO7)");
    vTaskDelay(pdMS_TO_TICKS(100));

    // 2. 初始化LVGL和墨水屏驱动
    ESP_LOGI(TAG, "Initializing LVGL and e-ink display...");
    if (!lvgl_eink_driver_init()) {
        ESP_LOGE(TAG, "LVGL/e-ink initialization failed!");
        return;
    }
    ESP_LOGI(TAG, "LVGL and e-ink display initialized");
    vTaskDelay(pdMS_TO_TICKS(500));

    // 3. 创建UI
    ESP_LOGI(TAG, "Creating sensor UI...");
    if (lvgl_lock()) {
        create_sensor_ui();
        lvgl_unlock();
    }

    // 等待LVGL任务处理UI渲染
    vTaskDelay(pdMS_TO_TICKS(200));

    // 首次全刷新显示UI
    ESP_LOGI(TAG, "Displaying initial UI...");
    if (lvgl_lock()) {
        lvgl_eink_set_refresh_mode(true);  // 全刷新
        lvgl_eink_force_refresh();
        lvgl_unlock();
    }
    ESP_LOGI(TAG, "UI created and displayed");
    vTaskDelay(pdMS_TO_TICKS(2000));

    // 4. 创建传感器任务
    ESP_LOGI(TAG, "Starting sensor task...");
    xTaskCreate(sensor_task, "sensor_task", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "═════════════════════════════════════════════");
    ESP_LOGI(TAG, "       System initialized successfully");
    ESP_LOGI(TAG, "═════════════════════════════════════════════");
    ESP_LOGI(TAG, "");
}
