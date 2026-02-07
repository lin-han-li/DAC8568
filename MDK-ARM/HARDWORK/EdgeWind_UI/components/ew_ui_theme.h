/**
 * @file ew_ui_theme.h
 * @brief EdgeWind UI 工业风主题（颜色/尺寸常量）
 */

#ifndef EW_UI_THEME_H
#define EW_UI_THEME_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

/* =========================
 * Colors (Gemini 工业浅色系)
 * ========================= */

#define EW_UI_COLOR_MAIN_BG     lv_color_hex(0xF4F7FA)
#define EW_UI_COLOR_CARD_BG     lv_color_hex(0xFFFFFF)
#define EW_UI_COLOR_TEXT_DARK   lv_color_hex(0x1F2937)
#define EW_UI_COLOR_TEXT_SOFT   lv_color_hex(0x6B7280)
#define EW_UI_COLOR_THEME       lv_color_hex(0x3B82F6)
#define EW_UI_COLOR_SAFE        lv_color_hex(0x10B981)

#define EW_UI_COLOR_BORDER      lv_color_hex(0xE5E7EB)
#define EW_UI_COLOR_PRESSED_BG  lv_color_hex(0xF3F4F6)
#define EW_UI_COLOR_SAFE_BG     lv_color_hex(0xECFDF5)
#define EW_UI_COLOR_THEME_BG    lv_color_hex(0xDBEAFE)

/* =========================
 * Layout metrics
 * ========================= */

#define EW_UI_HEADER_H          44
#define EW_UI_FOOTER_H          34

#define EW_UI_BODY_PAD          8
#define EW_UI_BODY_GAP          8

/* Card */
#define EW_UI_CARD_RADIUS       8

/* Pressed feedback (LVGL transform scale: 256 = 1.0) */
#define EW_UI_PRESSED_SCALE     248 /* ~0.97 */

/* Footer log */
#define EW_UI_LOG_MAX_LEN       160

static inline int32_t ew_ui_theme_screen_w(void)
{
    return (int32_t)lv_display_get_horizontal_resolution(NULL);
}

static inline int32_t ew_ui_theme_screen_h(void)
{
    return (int32_t)lv_display_get_vertical_resolution(NULL);
}

#ifdef __cplusplus
}
#endif

#endif /* EW_UI_THEME_H */
