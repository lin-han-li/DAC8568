/**
 * @file ew_ui_styles.h
 * @brief EdgeWind UI 样式表（与 screen 逻辑解耦）
 */

#ifndef EW_UI_STYLES_H
#define EW_UI_STYLES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

/* Global reusable styles (init once) */
extern lv_style_t ew_style_screen_bg;
extern lv_style_t ew_style_header;

extern lv_style_t ew_style_card_main;
extern lv_style_t ew_style_card_pressed_base;

extern lv_style_t ew_style_footer;

extern lv_style_t ew_style_text_title;
extern lv_style_t ew_style_text_cn;
extern lv_style_t ew_style_text_en;
extern lv_style_t ew_style_text_footer;

extern lv_style_t ew_style_badge;
extern lv_style_t ew_style_badge_text;

extern lv_style_t ew_style_back_btn;
extern lv_style_t ew_style_back_btn_pressed;

void ew_ui_styles_init_once(void);

#ifdef __cplusplus
}
#endif

#endif /* EW_UI_STYLES_H */

