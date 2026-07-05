/**
 * @file ew_ui_styles_redesign.c
 * @brief EdgeWind UI 重设计样式实现 - 温暖现代风格
 *
 * 核心设计原则：
 * 1. 温暖配色 - 奶油背景 + 土陶橘强调色
 * 2. 精致细节 - 柔和阴影 + 微妙圆角
 * 3. 信息层次 - 清晰的视觉权重
 * 4. 触感反馈 - 平滑的按压动画
 */

#include "ew_ui_styles_redesign.h"
#include "ew_ui_theme_redesign.h"
#include "../fonts/ew_fonts.h"

#include <stdbool.h>

/* 全局样式实例 */
lv_style_t ew_style_screen_bg_warm;
lv_style_t ew_style_header_warm;
lv_style_t ew_style_footer_warm;

lv_style_t ew_style_card_modern;
lv_style_t ew_style_card_pressed_modern;
lv_style_t ew_style_card_focused;

lv_style_t ew_style_btn_primary;
lv_style_t ew_style_btn_primary_pressed;
lv_style_t ew_style_btn_secondary;
lv_style_t ew_style_btn_secondary_pressed;

lv_style_t ew_style_badge_pill;
lv_style_t ew_style_badge_text_small;

lv_style_t ew_style_title_serif;
lv_style_t ew_style_text_body;
lv_style_t ew_style_text_caption;
lv_style_t ew_style_text_accent;

lv_style_t ew_style_status_dot;
lv_style_t ew_style_divider_line;

static bool s_styles_inited = false;

void ew_ui_styles_redesign_init(void)
{
    if (s_styles_inited) return;
    s_styles_inited = true;

    /* =========================
     * 屏幕背景
     * ========================= */
    lv_style_init(&ew_style_screen_bg_warm);
    lv_style_set_bg_color(&ew_style_screen_bg_warm, EW_COLOR_BG_CREAM);
    lv_style_set_bg_opa(&ew_style_screen_bg_warm, LV_OPA_COVER);
    lv_style_set_pad_all(&ew_style_screen_bg_warm, 0);

    /* =========================
     * 页眉样式 - 温暖白底 + 柔和阴影
     * ========================= */
    lv_style_init(&ew_style_header_warm);
    lv_style_set_bg_color(&ew_style_header_warm, EW_COLOR_SURFACE_WHITE);
    lv_style_set_bg_opa(&ew_style_header_warm, LV_OPA_COVER);
    lv_style_set_pad_hor(&ew_style_header_warm, EW_LAYOUT_HEADER_PAD_H);
    lv_style_set_pad_ver(&ew_style_header_warm, EW_LAYOUT_HEADER_PAD_V);
    lv_style_set_border_side(&ew_style_header_warm, LV_BORDER_SIDE_BOTTOM);
    lv_style_set_border_width(&ew_style_header_warm, 1);
    lv_style_set_border_color(&ew_style_header_warm, EW_COLOR_BORDER_WARM);
    lv_style_set_shadow_width(&ew_style_header_warm, 8);
    lv_style_set_shadow_ofs_y(&ew_style_header_warm, 2);
    lv_style_set_shadow_color(&ew_style_header_warm, lv_color_hex(0x000000));
    lv_style_set_shadow_opa(&ew_style_header_warm, LV_OPA_5);

    /* =========================
     * 页脚样式 - 浅奶油底色
     * ========================= */
    lv_style_init(&ew_style_footer_warm);
    lv_style_set_bg_color(&ew_style_footer_warm, EW_COLOR_SURFACE_LIGHT);
    lv_style_set_bg_opa(&ew_style_footer_warm, LV_OPA_COVER);
    lv_style_set_pad_hor(&ew_style_footer_warm, EW_LAYOUT_FOOTER_PAD_H);
    lv_style_set_pad_ver(&ew_style_footer_warm, EW_LAYOUT_FOOTER_PAD_V);
    lv_style_set_border_side(&ew_style_footer_warm, LV_BORDER_SIDE_TOP);
    lv_style_set_border_width(&ew_style_footer_warm, 1);
    lv_style_set_border_color(&ew_style_footer_warm, EW_COLOR_BORDER_WARM);

    /* =========================
     * 现代卡片 - 圆润 + 柔和阴影
     * ========================= */
    lv_style_init(&ew_style_card_modern);
    lv_style_set_bg_color(&ew_style_card_modern, EW_COLOR_SURFACE_WHITE);
    lv_style_set_bg_opa(&ew_style_card_modern, LV_OPA_COVER);
    lv_style_set_radius(&ew_style_card_modern, EW_LAYOUT_CARD_RADIUS);
    lv_style_set_border_width(&ew_style_card_modern, 1);
    lv_style_set_border_color(&ew_style_card_modern, EW_COLOR_BORDER_WARM);
    lv_style_set_shadow_width(&ew_style_card_modern, EW_SHADOW_CARD_WIDTH);
    lv_style_set_shadow_ofs_y(&ew_style_card_modern, EW_SHADOW_CARD_OFS_Y);
    lv_style_set_shadow_color(&ew_style_card_modern, lv_color_hex(0x000000));
    lv_style_set_shadow_opa(&ew_style_card_modern, EW_SHADOW_CARD_OPA);
    lv_style_set_pad_all(&ew_style_card_modern, EW_LAYOUT_CARD_PAD);

    /* 卡片按压效果 - 微妙提升 */
    lv_style_init(&ew_style_card_pressed_modern);
    lv_style_set_bg_color(&ew_style_card_pressed_modern, EW_COLOR_ACCENT_LIGHT);
    lv_style_set_transform_scale(&ew_style_card_pressed_modern, EW_PRESS_SCALE);
    lv_style_set_shadow_width(&ew_style_card_pressed_modern, EW_SHADOW_HOVER_WIDTH);
    lv_style_set_shadow_ofs_y(&ew_style_card_pressed_modern, EW_SHADOW_HOVER_OFS_Y);
    lv_style_set_shadow_opa(&ew_style_card_pressed_modern, EW_SHADOW_HOVER_OPA);

    /* 卡片焦点样式 - 土陶色边框 */
    lv_style_init(&ew_style_card_focused);
    lv_style_set_border_width(&ew_style_card_focused, 2);
    lv_style_set_border_color(&ew_style_card_focused, EW_COLOR_ACCENT_TERRA);
    lv_style_set_outline_width(&ew_style_card_focused, 3);
    lv_style_set_outline_pad(&ew_style_card_focused, 2);
    lv_style_set_outline_color(&ew_style_card_focused, EW_COLOR_ACCENT_TINT);
    lv_style_set_outline_opa(&ew_style_card_focused, LV_OPA_60);

    /* =========================
     * 主要按钮 - 土陶橘填充
     * ========================= */
    lv_style_init(&ew_style_btn_primary);
    lv_style_set_bg_color(&ew_style_btn_primary, EW_COLOR_ACCENT_TERRA);
    lv_style_set_bg_opa(&ew_style_btn_primary, LV_OPA_COVER);
    lv_style_set_radius(&ew_style_btn_primary, EW_LAYOUT_BTN_RADIUS);
    lv_style_set_border_width(&ew_style_btn_primary, 0);
    lv_style_set_pad_hor(&ew_style_btn_primary, EW_LAYOUT_BTN_PAD_H);
    lv_style_set_pad_ver(&ew_style_btn_primary, EW_LAYOUT_BTN_PAD_V);
    lv_style_set_shadow_width(&ew_style_btn_primary, 8);
    lv_style_set_shadow_ofs_y(&ew_style_btn_primary, 2);
    lv_style_set_shadow_color(&ew_style_btn_primary, EW_COLOR_ACCENT_TERRA);
    lv_style_set_shadow_opa(&ew_style_btn_primary, LV_OPA_30);

    lv_style_init(&ew_style_btn_primary_pressed);
    lv_style_set_bg_color(&ew_style_btn_primary_pressed, EW_COLOR_ACCENT_HOVER);
    lv_style_set_transform_scale(&ew_style_btn_primary_pressed, EW_PRESS_SCALE);
    lv_style_set_shadow_width(&ew_style_btn_primary_pressed, 12);
    lv_style_set_shadow_ofs_y(&ew_style_btn_primary_pressed, 4);
    lv_style_set_shadow_opa(&ew_style_btn_primary_pressed, LV_OPA_40);

    /* =========================
     * 次要按钮 - 白底土陶橘边框
     * ========================= */
    lv_style_init(&ew_style_btn_secondary);
    lv_style_set_bg_color(&ew_style_btn_secondary, EW_COLOR_SURFACE_WHITE);
    lv_style_set_bg_opa(&ew_style_btn_secondary, LV_OPA_COVER);
    lv_style_set_radius(&ew_style_btn_secondary, EW_LAYOUT_BTN_RADIUS);
    lv_style_set_border_width(&ew_style_btn_secondary, 1);
    lv_style_set_border_color(&ew_style_btn_secondary, EW_COLOR_ACCENT_TERRA);
    lv_style_set_pad_hor(&ew_style_btn_secondary, EW_LAYOUT_BTN_PAD_H);
    lv_style_set_pad_ver(&ew_style_btn_secondary, EW_LAYOUT_BTN_PAD_V);

    lv_style_init(&ew_style_btn_secondary_pressed);
    lv_style_set_bg_color(&ew_style_btn_secondary_pressed, EW_COLOR_ACCENT_TINT);
    lv_style_set_transform_scale(&ew_style_btn_secondary_pressed, EW_PRESS_SCALE);

    /* =========================
     * Pill徽章 - 完全圆角
     * ========================= */
    lv_style_init(&ew_style_badge_pill);
    lv_style_set_bg_color(&ew_style_badge_pill, EW_COLOR_SUCCESS_BG);
    lv_style_set_bg_opa(&ew_style_badge_pill, LV_OPA_COVER);
    lv_style_set_radius(&ew_style_badge_pill, EW_LAYOUT_BADGE_RADIUS);
    lv_style_set_border_width(&ew_style_badge_pill, 1);
    lv_style_set_border_color(&ew_style_badge_pill, EW_COLOR_SUCCESS_GREEN);
    lv_style_set_border_opa(&ew_style_badge_pill, LV_OPA_20);
    lv_style_set_pad_hor(&ew_style_badge_pill, EW_LAYOUT_BADGE_PAD_H);
    lv_style_set_pad_ver(&ew_style_badge_pill, EW_LAYOUT_BADGE_PAD_V);

    lv_style_init(&ew_style_badge_text_small);
    lv_style_set_text_font(&ew_style_badge_text_small, EW_FONT_CN_12);
    lv_style_set_text_color(&ew_style_badge_text_small, EW_COLOR_SUCCESS_GREEN);

    /* =========================
     * 文字样式
     * ========================= */
    /* Serif标题 - 温暖墨色 */
    lv_style_init(&ew_style_title_serif);
    lv_style_set_text_font(&ew_style_title_serif, EW_FONT_CN_16);
    lv_style_set_text_color(&ew_style_title_serif, EW_COLOR_INK_DARK);
    lv_style_set_text_letter_space(&ew_style_title_serif, -1); /* 紧凑字距 */

    /* 正文 - Inter轻量 */
    lv_style_init(&ew_style_text_body);
    lv_style_set_text_font(&ew_style_text_body, EW_FONT_CN_14);
    lv_style_set_text_color(&ew_style_text_body, EW_COLOR_INK_DARK);

    /* 辅助文字 - 柔和色 */
    lv_style_init(&ew_style_text_caption);
    lv_style_set_text_font(&ew_style_text_caption, EW_FONT_CN_12);
    lv_style_set_text_color(&ew_style_text_caption, EW_COLOR_TEXT_MUTED);

    /* 强调文字 - 土陶橘 */
    lv_style_init(&ew_style_text_accent);
    lv_style_set_text_font(&ew_style_text_accent, EW_FONT_CN_14);
    lv_style_set_text_color(&ew_style_text_accent, EW_COLOR_ACCENT_TERRA);

    /* =========================
     * 装饰元素
     * ========================= */
    /* 状态点 */
    lv_style_init(&ew_style_status_dot);
    lv_style_set_radius(&ew_style_status_dot, LV_RADIUS_CIRCLE);
    lv_style_set_bg_color(&ew_style_status_dot, EW_COLOR_SUCCESS_GREEN);
    lv_style_set_bg_opa(&ew_style_status_dot, LV_OPA_COVER);

    /* 分隔线 */
    lv_style_init(&ew_style_divider_line);
    lv_style_set_bg_color(&ew_style_divider_line, EW_COLOR_DIVIDER_SOFT);
    lv_style_set_bg_opa(&ew_style_divider_line, LV_OPA_COVER);
}

/* 根据故障类型获取对应颜色 */
lv_color_t ew_ui_get_fault_color(uint8_t fault_id)
{
    switch (fault_id) {
        case 0: return EW_FAULT_COLOR_AC;       /* 交流窜入 */
        case 1: return EW_FAULT_COLOR_INSUL;    /* 绝缘劣化 */
        case 2: return EW_FAULT_COLOR_CAP;      /* 电容老化 */
        case 3: return EW_FAULT_COLOR_IGBT;     /* IGBT故障 */
        case 4: return EW_FAULT_COLOR_BUS;      /* 母线接地 */
        case 5: return EW_FAULT_COLOR_PWM;      /* PWM异常 */
        default: return EW_COLOR_ACCENT_TERRA;
    }
}

/* 获取故障类型的浅色背景 */
lv_color_t ew_ui_get_fault_bg_color(uint8_t fault_id)
{
    /* 为每个故障色生成对应的浅色背景 */
    lv_color_t base = ew_ui_get_fault_color(fault_id);
    /* 简单的混合算法：与白色混合 */
    lv_color_t white = lv_color_hex(0xFFFFFF);

    /* 混合比例 20% 原色 + 80% 白色 */
    uint8_t r = (base.red * 20 + white.red * 80) / 100;
    uint8_t g = (base.green * 20 + white.green * 80) / 100;
    uint8_t b = (base.blue * 20 + white.blue * 80) / 100;

    return lv_color_make(r, g, b);
}
