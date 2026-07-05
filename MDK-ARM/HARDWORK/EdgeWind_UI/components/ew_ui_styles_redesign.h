/**
 * @file ew_ui_styles_redesign.h
 * @brief EdgeWind UI 重设计样式头文件
 */

#ifndef EW_UI_STYLES_REDESIGN_H
#define EW_UI_STYLES_REDESIGN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

/* 全局样式实例声明 */
extern lv_style_t ew_style_screen_bg_warm;
extern lv_style_t ew_style_header_warm;
extern lv_style_t ew_style_footer_warm;

extern lv_style_t ew_style_card_modern;
extern lv_style_t ew_style_card_pressed_modern;
extern lv_style_t ew_style_card_focused;

extern lv_style_t ew_style_btn_primary;
extern lv_style_t ew_style_btn_primary_pressed;
extern lv_style_t ew_style_btn_secondary;
extern lv_style_t ew_style_btn_secondary_pressed;

extern lv_style_t ew_style_badge_pill;
extern lv_style_t ew_style_badge_text_small;

extern lv_style_t ew_style_title_serif;
extern lv_style_t ew_style_text_body;
extern lv_style_t ew_style_text_caption;
extern lv_style_t ew_style_text_accent;

extern lv_style_t ew_style_status_dot;
extern lv_style_t ew_style_divider_line;

/**
 * @brief 初始化所有重设计样式（幂等）
 */
void ew_ui_styles_redesign_init(void);

/**
 * @brief 根据故障类型获取对应颜色
 * @param fault_id 故障ID (0..5)
 * @return 故障对应的主题色
 */
lv_color_t ew_ui_get_fault_color(uint8_t fault_id);

/**
 * @brief 获取故障类型的浅色背景
 * @param fault_id 故障ID (0..5)
 * @return 故障对应的背景色
 */
lv_color_t ew_ui_get_fault_bg_color(uint8_t fault_id);

#ifdef __cplusplus
}
#endif

#endif /* EW_UI_STYLES_REDESIGN_H */
