#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "display.h"

// ============================================================
// 绘制填充圆
// ============================================================
static void draw_filled_circle(esp_lcd_panel_handle_t panel,
                               int cx, int cy, int r, uint16_t color)
{
    if (!panel || r <= 0) return;

    int y_start = cy - r;
    int y_end   = cy + r;
    if (y_start < 0) y_start = 0;
    if (y_end >= LCD_V_RES) y_end = LCD_V_RES - 1;

    for (int y = y_start; y <= y_end; y++) {
        int dy = y - cy;
        int half_w = (int)sqrtf((float)(r * r - dy * dy));
        int x_start = cx - half_w;
        int x_end   = cx + half_w;
        if (x_start < 0) x_start = 0;
        if (x_end >= LCD_H_RES) x_end = LCD_H_RES - 1;

        int w = x_end - x_start + 1;
        if (w <= 0) continue;

        uint16_t *line = malloc(w * sizeof(uint16_t));
        if (!line) continue;
        for (int i = 0; i < w; i++) line[i] = color;
        esp_lcd_panel_draw_bitmap(panel, x_start, y, x_end + 1, y + 1, line);
        free(line);
    }
}

// ============================================================
// 绘制空心圆环
// ============================================================
static void draw_ring(esp_lcd_panel_handle_t panel,
                      int cx, int cy, int r, int thickness, uint16_t color)
{
    draw_filled_circle(panel, cx, cy, r, color);
    if (r > thickness) {
        draw_filled_circle(panel, cx, cy, r - thickness, COLOR_BLACK);
    }
}

// ============================================================
// 主函数
// ============================================================
void app_main(void)
{
    printf("\n");
    printf("========================================\n");
    printf("  KismetEngine - ST77916 Display Test\n");
    printf("  Round LCD: 360x360 SPI Mode\n");
    printf("========================================\n\n");

    // ====== 背光闪烁诊断 ======
    printf("=== Backlight Diagnostic ===\n");
    gpio_config_t bl_diag = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << PIN_LCD_BL,
    };
    gpio_config(&bl_diag);

    for (int i = 0; i < 3; i++) {
        gpio_set_level(PIN_LCD_BL, 0);  // LOW = 亮
        vTaskDelay(pdMS_TO_TICKS(200));
        gpio_set_level(PIN_LCD_BL, 1);  // HIGH = 灭
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    gpio_set_level(PIN_LCD_BL, 0);  // 保持亮

    // ====== 初始化显示屏 ======
    printf("Initializing display...\n");
    esp_lcd_panel_handle_t panel = display_init();

    if (!panel) {
        printf("ERROR: Display init failed!\n");
        printf("Wiring: SCK→IO%d MOSI→IO%d DC→IO%d CS→IO%d RST→IO%d BL→IO%d\n",
               PIN_LCD_SCLK, PIN_LCD_MOSI, PIN_LCD_DC, PIN_LCD_CS,
               PIN_LCD_RST, PIN_LCD_BL);
        return;
    }

    printf("\n=== Display Test ===\n");

    int cx = LCD_H_RES / 2;  // 180
    int cy = LCD_V_RES / 2;  // 180

    while (1) {
        // ---- 测试1: 整屏纯色填充 (每种2秒) ----
        printf("  RED\n");
        display_fill(panel, COLOR_RED);
        vTaskDelay(pdMS_TO_TICKS(2000));

        printf("  GREEN\n");
        display_fill(panel, COLOR_GREEN);
        vTaskDelay(pdMS_TO_TICKS(2000));

        printf("  BLUE\n");
        display_fill(panel, COLOR_BLUE);
        vTaskDelay(pdMS_TO_TICKS(2000));

        printf("  WHITE\n");
        display_fill(panel, COLOR_WHITE);
        vTaskDelay(pdMS_TO_TICKS(2000));

        // ---- 测试2: 棋盘格测试(验证像素映射) ----
        printf("  Checkerboard\n");
        int grid = 36;
        for (int row = 0; row < LCD_V_RES; row += grid) {
            for (int col = 0; col < LCD_H_RES; col += grid) {
                uint16_t c = ((row / grid + col / grid) % 2 == 0) ? COLOR_BLACK : COLOR_WHITE;
                int w = (col + grid > LCD_H_RES) ? LCD_H_RES - col : grid;
                int h = (row + grid > LCD_V_RES) ? LCD_V_RES - row : grid;
                display_fill_rect(panel, col, row, w, h, c);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(3000));

        // ---- 测试3: 彩色同心圆 (粗环,高对比度) ----
        printf("  Rings\n");
        display_fill(panel, COLOR_BLACK);
        draw_filled_circle(panel, cx, cy, 180, COLOR_RED);       // 最外层: 红
        vTaskDelay(pdMS_TO_TICKS(400));
        draw_filled_circle(panel, cx, cy, 140, COLOR_BLACK);
        draw_ring(panel, cx, cy, 140, 20, COLOR_GREEN);          // 绿环
        vTaskDelay(pdMS_TO_TICKS(400));
        draw_filled_circle(panel, cx, cy, 90, COLOR_BLACK);
        draw_ring(panel, cx, cy, 90, 20, COLOR_BLUE);            // 蓝环
        vTaskDelay(pdMS_TO_TICKS(400));
        draw_filled_circle(panel, cx, cy, 40, COLOR_BLACK);
        draw_filled_circle(panel, cx, cy, 40, COLOR_YELLOW);     // 中心: 黄
        vTaskDelay(pdMS_TO_TICKS(400));
        draw_filled_circle(panel, cx, cy, 15, COLOR_WHITE);      // 中心白点
        vTaskDelay(pdMS_TO_TICKS(3000));

        // ---- 测试4: 简单扫描线 ----
        printf("  Sweep\n");
        for (int angle = 0; angle < 360; angle += 3) {
            // 用黑底+白线，清晰可见
            display_fill(panel, COLOR_BLACK);

            // 画两个参考环
            draw_ring(panel, cx, cy, 160, 4, RGB565(40, 40, 80));
            draw_ring(panel, cx, cy, 80, 4, RGB565(40, 40, 80));

            // 白色扫描线
            float rad = angle * 3.14159f / 180.0f;
            int ex = cx + (int)(170 * cosf(rad));
            int ey = cy + (int)(170 * sinf(rad));

            // 简单画线 (Bresenham)
            int dx = abs(ex - cx), sx = (cx < ex) ? 1 : -1;
            int dy = -abs(ey - cy), sy = (cy < ey) ? 1 : -1;
            int err = dx + dy;
            int x = cx, y = cy;
            while (1) {
                if (x >= 0 && x < LCD_H_RES && y >= 0 && y < LCD_V_RES) {
                    uint16_t pixel = COLOR_WHITE;
                    esp_lcd_panel_draw_bitmap(panel, x, y, x + 1, y + 1, &pixel);
                }
                if (x == ex && y == ey) break;
                int e2 = 2 * err;
                if (e2 >= dy) { err += dy; x += sx; }
                if (e2 <= dx) { err += dx; y += sy; }
            }
            vTaskDelay(pdMS_TO_TICKS(20));
        }

        printf("  Loop complete. Restarting...\n\n");
    }
}
