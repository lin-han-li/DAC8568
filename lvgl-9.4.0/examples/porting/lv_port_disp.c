/**
 * @file lv_port_disp.c
 */

/* Copy this file as "lv_port_disp.c" and set this value to "1" to enable content */
#if 1

/*********************
 * INCLUDES
 *********************/
#include "lv_port_disp.h"
#include "lcd_spi_200.h"
#include <stdbool.h>
#include <stdint.h>
#if LV_USE_SYSMON
#include "lv_sysmon.h"
#endif

/*********************
 * DEFINES
 *********************/
#ifndef LVGL_SPI_LCD_BUF_LINES
#define LVGL_SPI_LCD_BUF_LINES 20
#endif

/* This project uses the SPI LCD in landscape (Direction_H): 320x240. */
#define LVGL_LCD_HOR_RES LCD_Height
#define LVGL_LCD_VER_RES LCD_Width

/**********************
 * STATIC PROTOTYPES
 **********************/
static void disp_init(void);
static void disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);

/**********************
 * STATIC VARIABLES
 **********************/
#if defined(__ARMCC_VERSION) || defined(__clang__) || defined(__GNUC__)
#define LVGL_DRAW_BUF_ALIGNED __attribute__((aligned(LV_DRAW_BUF_ALIGN)))
#else
#define LVGL_DRAW_BUF_ALIGNED
#endif

/* LVGL requires draw buffer base address alignment to LV_DRAW_BUF_ALIGN (default: 4). */
static lv_color16_t s_disp_buf_1[LVGL_LCD_HOR_RES * LVGL_SPI_LCD_BUF_LINES] LVGL_DRAW_BUF_ALIGNED;
static lv_color16_t s_disp_buf_2[LVGL_LCD_HOR_RES * LVGL_SPI_LCD_BUF_LINES] LVGL_DRAW_BUF_ALIGNED;

/**********************
 * GLOBAL VARIABLES
 **********************/
volatile bool disp_flush_enabled = true;
volatile uint32_t g_lvgl_disp_flush_count = 0u;

/**********************
 * GLOBAL FUNCTIONS
 **********************/
void lv_port_disp_init(void)
{
    disp_init();

    lv_display_t *disp = lv_display_create(LVGL_LCD_HOR_RES, LVGL_LCD_VER_RES);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(disp, disp_flush);
    lv_display_set_buffers(
        disp,
        s_disp_buf_1,
        s_disp_buf_2,
        sizeof(s_disp_buf_1),
        LV_DISPLAY_RENDER_MODE_PARTIAL);

#if LV_USE_SYSMON
    /* Disable on-screen performance/memory monitors (FPS/CPU/heap overlay). */
    lv_sysmon_hide_performance(disp);
#endif
}

void disp_enable_update(void)
{
    disp_flush_enabled = true;
}

void disp_disable_update(void)
{
    disp_flush_enabled = false;
}

/**********************
 * STATIC FUNCTIONS
 **********************/
static void disp_init(void)
{
    /* Initialize SPI LCD after scheduler start to avoid blocking boot path. */
    SPI_LCD_Init();
}

static void disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    g_lvgl_disp_flush_count++;

    if (disp_flush_enabled)
    {
        uint16_t x = (uint16_t)area->x1;
        uint16_t y = (uint16_t)area->y1;
        uint16_t width = (uint16_t)(area->x2 - area->x1 + 1);
        uint16_t height = (uint16_t)(area->y2 - area->y1 + 1);

        LCD_CopyBuffer(x, y, width, height, (uint16_t *)px_map);
    }

    lv_display_flush_ready(disp);
}

#else /* Enable this file at the top */

/* This dummy typedef exists purely to silence -Wpedantic. */
typedef int keep_pedantic_happy;
#endif
