/**
 * @file ew_ui_styles.c
 * @brief EdgeWind UI 样式表实现（幂等初始化）
 */

#include "ew_ui_styles.h"

#include "ew_ui_theme.h"

#include "../fonts/ew_fonts.h"

#include <stdbool.h>

static bool s_inited = false;

lv_style_t ew_style_screen_bg;
lv_style_t ew_style_header;

lv_style_t ew_style_card_main;
lv_style_t ew_style_card_pressed_base;

lv_style_t ew_style_footer;

lv_style_t ew_style_text_title;
lv_style_t ew_style_text_cn;
lv_style_t ew_style_text_en;
lv_style_t ew_style_text_footer;

lv_style_t ew_style_badge;
lv_style_t ew_style_badge_text;

lv_style_t ew_style_back_btn;
lv_style_t ew_style_back_btn_pressed;

void ew_ui_styles_init_once(void)
{
    if (s_inited) return;
    s_inited = true;

    /* Screen background */
    lv_style_init(&ew_style_screen_bg);
    lv_style_set_bg_color(&ew_style_screen_bg, EW_UI_COLOR_MAIN_BG);
    lv_style_set_bg_opa(&ew_style_screen_bg, LV_OPA_COVER);
    lv_style_set_pad_all(&ew_style_screen_bg, 0);

    /* Header */
    lv_style_init(&ew_style_header);
    lv_style_set_bg_color(&ew_style_header, EW_UI_COLOR_CARD_BG);
    lv_style_set_bg_opa(&ew_style_header, LV_OPA_COVER);
    lv_style_set_pad_hor(&ew_style_header, EW_UI_BODY_PAD);
    lv_style_set_pad_ver(&ew_style_header, 0);
    lv_style_set_border_side(&ew_style_header, LV_BORDER_SIDE_BOTTOM);
    lv_style_set_border_width(&ew_style_header, 1);
    lv_style_set_border_color(&ew_style_header, EW_UI_COLOR_BORDER);
    lv_style_set_shadow_width(&ew_style_header, 12);
    lv_style_set_shadow_ofs_y(&ew_style_header, 4);
    lv_style_set_shadow_color(&ew_style_header, lv_color_hex(0x000000));
    lv_style_set_shadow_opa(&ew_style_header, LV_OPA_10);

    /* Card main */
    lv_style_init(&ew_style_card_main);
    lv_style_set_bg_color(&ew_style_card_main, EW_UI_COLOR_CARD_BG);
    lv_style_set_bg_opa(&ew_style_card_main, LV_OPA_COVER);
    lv_style_set_radius(&ew_style_card_main, EW_UI_CARD_RADIUS);
    lv_style_set_border_width(&ew_style_card_main, 1);
    lv_style_set_border_color(&ew_style_card_main, EW_UI_COLOR_BORDER);
    lv_style_set_shadow_width(&ew_style_card_main, 18);
    lv_style_set_shadow_ofs_y(&ew_style_card_main, 8);
    lv_style_set_shadow_color(&ew_style_card_main, lv_color_hex(0x000000));
    lv_style_set_shadow_opa(&ew_style_card_main, LV_OPA_10);
    lv_style_set_pad_all(&ew_style_card_main, 0);
    lv_style_set_clip_corner(&ew_style_card_main, true);

    /* Card pressed feedback (base, per-card theme color is applied on objects) */
    lv_style_init(&ew_style_card_pressed_base);
    lv_style_set_bg_color(&ew_style_card_pressed_base, EW_UI_COLOR_PRESSED_BG);
    lv_style_set_transform_scale(&ew_style_card_pressed_base, EW_UI_PRESSED_SCALE);
    lv_style_set_shadow_width(&ew_style_card_pressed_base, 20);
    lv_style_set_shadow_ofs_y(&ew_style_card_pressed_base, 4);
    lv_style_set_shadow_opa(&ew_style_card_pressed_base, LV_OPA_20);

    /* Footer */
    lv_style_init(&ew_style_footer);
    lv_style_set_bg_color(&ew_style_footer, EW_UI_COLOR_CARD_BG);
    lv_style_set_bg_opa(&ew_style_footer, LV_OPA_COVER);
    lv_style_set_border_side(&ew_style_footer, LV_BORDER_SIDE_TOP);
    lv_style_set_border_width(&ew_style_footer, 1);
    lv_style_set_border_color(&ew_style_footer, EW_UI_COLOR_BORDER);
    lv_style_set_pad_hor(&ew_style_footer, EW_UI_BODY_PAD);
    lv_style_set_pad_ver(&ew_style_footer, 6);

    /* Text styles */
    lv_style_init(&ew_style_text_title);
    lv_style_set_text_font(&ew_style_text_title, EW_FONT_CN_16);
    lv_style_set_text_color(&ew_style_text_title, EW_UI_COLOR_TEXT_DARK);

    lv_style_init(&ew_style_text_cn);
    lv_style_set_text_font(&ew_style_text_cn, EW_FONT_CN_16);
    lv_style_set_text_color(&ew_style_text_cn, EW_UI_COLOR_TEXT_DARK);

    lv_style_init(&ew_style_text_en);
    lv_style_set_text_font(&ew_style_text_en, &lv_font_montserrat_12);
    lv_style_set_text_color(&ew_style_text_en, EW_UI_COLOR_TEXT_SOFT);

    lv_style_init(&ew_style_text_footer);
    lv_style_set_text_font(&ew_style_text_footer, EW_FONT_CN_12);
    lv_style_set_text_color(&ew_style_text_footer, EW_UI_COLOR_TEXT_SOFT);

    /* Status badge */
    lv_style_init(&ew_style_badge);
    lv_style_set_bg_color(&ew_style_badge, EW_UI_COLOR_SAFE_BG);
    lv_style_set_bg_opa(&ew_style_badge, LV_OPA_COVER);
    lv_style_set_border_width(&ew_style_badge, 1);
    lv_style_set_border_color(&ew_style_badge, EW_UI_COLOR_SAFE);
    lv_style_set_radius(&ew_style_badge, LV_RADIUS_CIRCLE);
    lv_style_set_pad_hor(&ew_style_badge, 12);
    lv_style_set_pad_ver(&ew_style_badge, 6);
    lv_style_set_pad_column(&ew_style_badge, 8);

    lv_style_init(&ew_style_badge_text);
    lv_style_set_text_font(&ew_style_badge_text, EW_FONT_CN_12);
    lv_style_set_text_color(&ew_style_badge_text, EW_UI_COLOR_SAFE);

    /* Back button (placeholder screen) */
    lv_style_init(&ew_style_back_btn);
    lv_style_set_bg_opa(&ew_style_back_btn, LV_OPA_TRANSP);
    lv_style_set_border_width(&ew_style_back_btn, 0);
    lv_style_set_pad_hor(&ew_style_back_btn, 10);
    lv_style_set_pad_ver(&ew_style_back_btn, 6);
    lv_style_set_radius(&ew_style_back_btn, 8);

    lv_style_init(&ew_style_back_btn_pressed);
    lv_style_set_bg_color(&ew_style_back_btn_pressed, EW_UI_COLOR_THEME_BG);
    lv_style_set_bg_opa(&ew_style_back_btn_pressed, LV_OPA_COVER);
    lv_style_set_transform_scale(&ew_style_back_btn_pressed, EW_UI_PRESSED_SCALE);
}

