#pragma once

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_commands.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// 引脚配置 — ST77916 圆形屏 (1.8" 360x360)
//
// 屏幕模块引脚        ESP32-S3 GPIO    说明
// ------------------------------------------------------------------
// VCC               →  3V3              电源
// GND               →  GND              地
// PCLK / SCK        →  IO21             SPI 时钟
// DATA0 / MOSI      →  IO47             SPI MOSI (数据)
// DATA1 / DC        →  IO40             SPI DC (命令/数据选择)
// DATA2 / NC        →  不接             (QSPI 模式才用)
// DATA3 / NC        →  不接             (QSPI 模式才用)
// CS                →  IO41             片选
// RST / RES         →  IO45             复位
// BLC               →  IO42             背光控制
// SCL (触摸)        →  IO12             触摸 I2C 时钟
// SDA (触摸)        →  IO11             触摸 I2C 数据
// RST (触摸复位)    →  IO13             触摸复位
// INT (触摸中断)    →  IO14             触摸中断
// ============================================================
#define PIN_LCD_SCLK   21   // → PCLK/SCK
#define PIN_LCD_DATA0  47   // → DATA0/MOSI (SPI MOSI)
#define PIN_LCD_DATA1  40   // → DATA1/DC   (SPI DC)
#define PIN_LCD_DATA2  46   // → 不接 (SPI 模式 NC)
#define PIN_LCD_DATA3  48   // → 不接 (SPI 模式 NC)
#define PIN_LCD_CS     41   // → CS
#define PIN_LCD_RST    45   // → RST/RES
#define PIN_LCD_BL     42   // → BLC (背光)

// 触摸
#define PIN_TOUCH_SCL  12   // → SCL
#define PIN_TOUCH_SDA  11   // → SDA
#define PIN_TOUCH_RST  13   // → RST (触摸复位)
#define PIN_TOUCH_INT  14   // → INT

// ============================================================
// 显示参数
// ============================================================
#define LCD_H_RES      360
#define LCD_V_RES      360
#define LCD_BITS_PER_PIXEL  16  // RGB565

// ============================================================
// 颜色定义 (RGB565)
// ============================================================
#define COLOR_BLACK       0x0000
#define COLOR_WHITE       0xFFFF
#define COLOR_RED         0xF800
#define COLOR_GREEN       0x07E0
#define COLOR_BLUE        0x001F
#define COLOR_YELLOW      0xFFE0
#define COLOR_CYAN        0x07FF
#define COLOR_MAGENTA     0xF81F
#define COLOR_ORANGE      0xFD20
#define COLOR_GRAY        0x8410

// RGB888 → RGB565
#define RGB565(r, g, b)   ((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3))

// ============================================================
// API
// ============================================================

/**
 * @brief 初始化 ST77916 SPI 显示屏
 * @return esp_lcd_panel_handle_t 面板句柄，失败返回 NULL
 */
esp_lcd_panel_handle_t display_init(void);

/**
 * @brief 填充整个屏幕为指定颜色
 */
void display_fill(esp_lcd_panel_handle_t panel, uint16_t color);

/**
 * @brief 填充矩形区域
 */
void display_fill_rect(
    esp_lcd_panel_handle_t panel,
    uint16_t x,
    uint16_t y,
    uint16_t w,
    uint16_t h,
    uint16_t color
);

/**
 * @brief 设置背光亮度 (0~100)
 */
void display_backlight_set(int brightness_percent);

#ifdef __cplusplus
}
#endif
