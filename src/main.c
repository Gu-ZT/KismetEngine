#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "display.h"

// 列扫描填充圆 (窄CASET,无拖影)
static void draw_filled_circle(esp_lcd_panel_handle_t panel,
                               int cx, int cy, int r, uint16_t color)
{
    if (!panel || r <= 0) return;
    int xs = cx - r, xe = cx + r;
    if (xs < 0) xs = 0;
    if (xe >= LCD_H_RES) xe = LCD_H_RES - 1;
    for (int x = xs; x <= xe; x++) {
        int half_h = (int)sqrtf((float)(r * r - (x - cx) * (x - cx)));
        int ys = cy - half_h, ye = cy + half_h;
        if (ys < 0) ys = 0;
        if (ye >= LCD_V_RES) ye = LCD_V_RES - 1;
        int h = ye - ys + 1;
        if (h <= 0) continue;
        uint16_t *col = malloc(h * sizeof(uint16_t));
        if (!col) continue;
        for (int i = 0; i < h; i++) col[i] = color;
        esp_lcd_panel_draw_bitmap(panel, x, ys, x + 1, ye + 1, col);
        free(col);
    }
}

static void draw_ring(esp_lcd_panel_handle_t panel,
                      int cx, int cy, int r, int thickness, uint16_t color)
{
    draw_filled_circle(panel, cx, cy, r, color);
    if (r > thickness)
        draw_filled_circle(panel, cx, cy, r - thickness, COLOR_BLACK);
}

void app_main(void)
{
    printf("\n=== ST77916 Rings Test (W180 Init) ===\n");

    esp_lcd_panel_handle_t panel = display_init();
    if (!panel) { printf("FAIL\n"); return; }

    int cx = 180, cy = 180;

    while (1) {
        printf("RINGS\n");
        display_fill(panel, COLOR_BLACK);

        draw_filled_circle(panel, cx, cy, 180, COLOR_RED);
        vTaskDelay(pdMS_TO_TICKS(300));

        draw_ring(panel, cx, cy, 150, 15, COLOR_GREEN);
        vTaskDelay(pdMS_TO_TICKS(300));

        draw_ring(panel, cx, cy, 100, 15, COLOR_BLUE);
        vTaskDelay(pdMS_TO_TICKS(300));

        draw_filled_circle(panel, cx, cy, 50, COLOR_BLACK);
        draw_filled_circle(panel, cx, cy, 50, COLOR_YELLOW);
        vTaskDelay(pdMS_TO_TICKS(300));
        draw_filled_circle(panel, cx, cy, 15, COLOR_WHITE);

        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}
