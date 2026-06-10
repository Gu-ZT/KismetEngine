#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "display.h"

// 行扫描填充圆
static void draw_filled_circle(esp_lcd_panel_handle_t panel,
                               int cx, int cy, int r, uint16_t color)
{
    if (!panel || r <= 0) return;
    int ys = cy - r, ye = cy + r;
    if (ys < 0) ys = 0;
    if (ye >= LCD_V_RES) ye = LCD_V_RES - 1;
    for (int y = ys; y <= ye; y++) {
        int half_w = (int)sqrtf((float)(r * r - (y - cy) * (y - cy)));
        int xs = cx - half_w, xe = cx + half_w;
        if (xs < 0) xs = 0;
        if (xe >= LCD_H_RES) xe = LCD_H_RES - 1;
        int w = xe - xs + 1;
        if (w <= 0) continue;
        uint16_t *line = malloc(w * sizeof(uint16_t));
        if (!line) continue;
        for (int i = 0; i < w; i++) line[i] = color;
        esp_lcd_panel_draw_bitmap(panel, xs, y, xe + 1, y + 1, line);
        free(line);
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

    gpio_config_t bl = {.mode = GPIO_MODE_OUTPUT, .pin_bit_mask = 1ULL << PIN_LCD_BL};
    gpio_config(&bl);
    gpio_set_level(PIN_LCD_BL, 0);

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
