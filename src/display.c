#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_st77916.h"
#include "display.h"

static const char *TAG = "display";

// SPI 主机选择: SPI2_HOST 或 SPI3_HOST
#define LCD_SPI_HOST  SPI3_HOST

// ============================================================
// SPI 总线初始化
// ============================================================
static bool spi_bus_init(void)
{
    const spi_bus_config_t buscfg = ST77916_PANEL_BUS_SPI_CONFIG(
        PIN_LCD_SCLK,      // SCLK
        PIN_LCD_MOSI,      // MOSI
        LCD_H_RES * 80 * sizeof(uint16_t)   // max transfer size
    );

    ESP_LOGI(TAG, "Initializing SPI bus (SPI%d_HOST): SCLK=IO%d MOSI=IO%d",
             LCD_SPI_HOST, PIN_LCD_SCLK, PIN_LCD_MOSI);

    esp_err_t ret = spi_bus_initialize(LCD_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus init failed: %s", esp_err_to_name(ret));
        return false;
    }
    return true;
}

// ============================================================
// 面板初始化
// ============================================================
esp_lcd_panel_handle_t display_init(void)
{
    esp_err_t ret;

    // 1. 背光 GPIO
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << PIN_LCD_BL,
    };
    gpio_config(&bk_gpio_config);
    gpio_set_level(PIN_LCD_BL, 0);  // LOW = 开背光 (低电平有效)
    ESP_LOGI(TAG, "Backlight ON (IO%d, active-low)", PIN_LCD_BL);

    // 2. 硬件复位
    gpio_config_t rst_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << PIN_LCD_RST,
    };
    gpio_config(&rst_gpio_config);

    ESP_LOGI(TAG, "Hardware reset: IO%d LOW", PIN_LCD_RST);
    gpio_set_level(PIN_LCD_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(PIN_LCD_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(150));
    ESP_LOGI(TAG, "Hardware reset complete");

    // 3. SPI 总线初始化
    if (!spi_bus_init()) {
        return NULL;
    }
    ESP_LOGI(TAG, "SPI bus initialized");

    // 4. 面板 IO 创建 (标准 SPI 模式: CS + DC)
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = ST77916_PANEL_IO_SPI_CONFIG(
        PIN_LCD_CS,         // CS
        PIN_LCD_DC,         // DC (命令/数据选择)
        NULL,               // 无回调
        NULL                // 无回调数据
    );
    // 降低时钟以防杜邦线信号衰减 (40MHz → 10MHz)
    // 使用 SPI Mode 3 (CPOL=1,CPHA=1)，部分 ST77916 模块需要
    io_config.pclk_hz = 40 * 1000 * 1000;
    io_config.spi_mode = 3;

    ESP_LOGI(TAG, "Panel IO: CS=IO%d DC=IO%d CLK=%dMHz MODE=%d",
             PIN_LCD_CS, PIN_LCD_DC,
             (int)(io_config.pclk_hz / 1000000), io_config.spi_mode);

    ret = esp_lcd_new_panel_io_spi(
        (esp_lcd_spi_bus_handle_t)LCD_SPI_HOST,
        &io_config,
        &io_handle
    );
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Panel IO creation failed: %s", esp_err_to_name(ret));
        return NULL;
    }
    ESP_LOGI(TAG, "Panel IO created (SPI mode)");

    // 5. ST77916 面板创建
    esp_lcd_panel_handle_t panel = NULL;

    const st77916_vendor_config_t vendor_config = {
        .flags = {
            .use_qspi_interface = 0,   // 标准 SPI 模式
        },
    };

    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = LCD_BITS_PER_PIXEL,
        .vendor_config = (void *)&vendor_config,
    };

    ret = esp_lcd_new_panel_st77916(io_handle, &panel_config, &panel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Panel creation failed: %s", esp_err_to_name(ret));
        return NULL;
    }
    ESP_LOGI(TAG, "ST77916 panel created");

    // 6. 面板复位 + 初始化
    ESP_LOGI(TAG, "Resetting panel...");
    ret = esp_lcd_panel_reset(panel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Panel reset failed: %s", esp_err_to_name(ret));
        return NULL;
    }

    ESP_LOGI(TAG, "Initializing panel (sending init commands)...");
    ret = esp_lcd_panel_init(panel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Panel init failed: %s", esp_err_to_name(ret));
        return NULL;
    }
    ESP_LOGI(TAG, "Panel init done");

    // 7. 开启显示
    ret = esp_lcd_panel_disp_on_off(panel, true);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Panel disp_on failed: %s", esp_err_to_name(ret));
        return NULL;
    }
    ESP_LOGI(TAG, "=== Display ON ===");

    return panel;
}

// ============================================================
// 填充整个屏幕
// ============================================================
void display_fill(esp_lcd_panel_handle_t panel, uint16_t color)
{
    if (!panel) return;

    // 分配一行缓冲区 (360 pixels × 2 bytes = 720 bytes)
    uint16_t *line_buf = malloc(LCD_H_RES * sizeof(uint16_t));
    if (!line_buf) {
        ESP_LOGE(TAG, "Failed to allocate line buffer");
        return;
    }

    for (int i = 0; i < LCD_H_RES; i++) {
        line_buf[i] = color;
    }

    for (int y = 0; y < LCD_V_RES; y++) {
        esp_lcd_panel_draw_bitmap(panel, 0, y, LCD_H_RES, y + 1, line_buf);
    }

    free(line_buf);
}

// ============================================================
// 填充矩形区域
// ============================================================
void display_fill_rect(esp_lcd_panel_handle_t panel,
                       uint16_t x, uint16_t y,
                       uint16_t w, uint16_t h,
                       uint16_t color)
{
    if (!panel) return;

    if (x >= LCD_H_RES || y >= LCD_V_RES) return;
    if (x + w > LCD_H_RES) w = LCD_H_RES - x;
    if (y + h > LCD_V_RES) h = LCD_V_RES - y;
    if (w == 0 || h == 0) return;

    uint16_t *line_buf = malloc(w * sizeof(uint16_t));
    if (!line_buf) return;

    for (int i = 0; i < w; i++) {
        line_buf[i] = color;
    }

    for (int row = 0; row < h; row++) {
        esp_lcd_panel_draw_bitmap(panel, x, y + row, x + w, y + row + 1, line_buf);
    }

    free(line_buf);
}

// ============================================================
// 背光控制
// ============================================================
void display_backlight_set(int brightness_percent)
{
    if (brightness_percent <= 0) {
        gpio_set_level(PIN_LCD_BL, 1);  // HIGH = 灭
    } else {
        gpio_set_level(PIN_LCD_BL, 0);  // LOW = 亮 (低电平有效)
    }
}
