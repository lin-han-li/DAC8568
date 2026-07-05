/**
 * @file ew_ui_theme_redesign.h
 * @brief EdgeWind UI 重设计 - 温暖现代风格主题
 *
 * 设计理念：
 * - 温暖的奶油色背景，避免冷色调蓝灰
 * - Editorial serif标题 + Inter无衬线字体
 * - 土陶色(Terracotta)强调色
 * - 柔和的阴影和微妙的提升效果
 */

#ifndef EW_UI_THEME_REDESIGN_H
#define EW_UI_THEME_REDESIGN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

/* =========================
 * 温暖配色系统
 * ========================= */

/* 背景层 - 温暖奶油色系 */
#define EW_COLOR_BG_CREAM       lv_color_hex(0xF7F4EF)  /* 温暖奶油白 */
#define EW_COLOR_SURFACE_LIGHT  lv_color_hex(0xFBF9F5)  /* 浅表面 */
#define EW_COLOR_SURFACE_WHITE  lv_color_hex(0xFFFFFF)  /* 纯白表面 */

/* 边框与分隔线 - 温暖发丝线 */
#define EW_COLOR_BORDER_WARM    lv_color_hex(0xE7E1D7)  /* 温暖发丝边框 */
#define EW_COLOR_DIVIDER_SOFT   lv_color_hex(0xEDE9E1)  /* 柔和分隔线 */

/* 文字层 - 温暖墨色 */
#define EW_COLOR_INK_DARK       lv_color_hex(0x1F2421)  /* 深墨色文字 */
#define EW_COLOR_TEXT_MUTED     lv_color_hex(0x5C635D)  /* 柔和文字 */
#define EW_COLOR_TEXT_SOFT      lv_color_hex(0x7D8380)  /* 次要文字 */

/* 强调色 - 土陶橘 */
#define EW_COLOR_ACCENT_TERRA   lv_color_hex(0xC4612F)  /* 土陶橘 */
#define EW_COLOR_ACCENT_HOVER   lv_color_hex(0xA94E22)  /* 土陶橘悬停 */
#define EW_COLOR_ACCENT_TINT    lv_color_hex(0xF2E3D6)  /* 土陶浅色 */
#define EW_COLOR_ACCENT_LIGHT   lv_color_hex(0xF8EDE3)  /* 土陶极浅 */

/* 功能色 */
#define EW_COLOR_SUCCESS_GREEN  lv_color_hex(0x4CAF50)  /* 成功绿 */
#define EW_COLOR_SUCCESS_BG     lv_color_hex(0xE8F5E9)  /* 成功背景 */
#define EW_COLOR_WARNING_AMBER  lv_color_hex(0xFF9800)  /* 警告琥珀 */
#define EW_COLOR_WARNING_BG     lv_color_hex(0xFFF3E0)  /* 警告背景 */
#define EW_COLOR_ERROR_RED      lv_color_hex(0xE53935)  /* 错误红 */
#define EW_COLOR_ERROR_BG       lv_color_hex(0xFFEBEE)  /* 错误背景 */

/* 深色区块 - 温暖炭色 */
#define EW_COLOR_CHARCOAL       lv_color_hex(0x1F2421)  /* 温暖炭色 */
#define EW_COLOR_CHARCOAL_SOFT  lv_color_hex(0x3A3F3C)  /* 浅炭色 */

/* =========================
 * 布局尺寸（320×240优化）
 * ========================= */

/* 页眉 */
#define EW_LAYOUT_HEADER_H      32      /* 页眉高度 */
#define EW_LAYOUT_HEADER_PAD_H  10      /* 页眉水平内边距 */
#define EW_LAYOUT_HEADER_PAD_V  6       /* 页眉垂直内边距 */

/* 页脚 */
#define EW_LAYOUT_FOOTER_H      24      /* 页脚高度 */
#define EW_LAYOUT_FOOTER_PAD_H  8       /* 页脚水平内边距 */
#define EW_LAYOUT_FOOTER_PAD_V  4       /* 页脚垂直内边距 */

/* 主体 */
#define EW_LAYOUT_BODY_PAD      10      /* 主体内边距 */
#define EW_LAYOUT_CARD_GAP      8       /* 卡片间距 */

/* 卡片 */
#define EW_LAYOUT_CARD_RADIUS   12      /* 卡片圆角（更圆润）*/
#define EW_LAYOUT_CARD_PAD      10      /* 卡片内边距 */
#define EW_LAYOUT_CARD_MIN_H    52      /* 卡片最小高度 */

/* 按钮 */
#define EW_LAYOUT_BTN_RADIUS    999     /* 完全圆角pill按钮 */
#define EW_LAYOUT_BTN_PAD_H     14      /* 按钮水平内边距 */
#define EW_LAYOUT_BTN_PAD_V     8       /* 按钮垂直内边距 */

/* 徽章/Chip */
#define EW_LAYOUT_BADGE_RADIUS  999     /* 完全圆角badge */
#define EW_LAYOUT_BADGE_PAD_H   8       /* badge水平内边距 */
#define EW_LAYOUT_BADGE_PAD_V   3       /* badge垂直内边距 */

/* =========================
 * 视觉效果
 * ========================= */

/* 阴影 */
#define EW_SHADOW_CARD_WIDTH    12      /* 卡片阴影宽度 */
#define EW_SHADOW_CARD_OFS_Y    3       /* 卡片阴影Y偏移 */
#define EW_SHADOW_CARD_OPA      LV_OPA_10  /* 卡片阴影不透明度 */

#define EW_SHADOW_HOVER_WIDTH   16      /* 悬停阴影宽度 */
#define EW_SHADOW_HOVER_OFS_Y   5       /* 悬停阴影Y偏移 */
#define EW_SHADOW_HOVER_OPA     LV_OPA_15  /* 悬停阴影不透明度 */

/* 按压效果 */
#define EW_PRESS_SCALE          250     /* 按压缩放 (~0.98x) */
#define EW_PRESS_LIFT_Y         -2      /* 按压时的Y位移 */

/* 过渡动画 */
#define EW_ANIM_DURATION_FAST   150     /* 快速动画（毫秒）*/
#define EW_ANIM_DURATION_NORMAL 250     /* 正常动画 */
#define EW_ANIM_DURATION_SLOW   400     /* 慢速动画 */

/* =========================
 * 故障类型配色
 * ========================= */

/* 每个故障类型对应一个温暖的强调色 */
#define EW_FAULT_COLOR_AC       lv_color_hex(0xE67E22)  /* 交流窜入 - 橙色 */
#define EW_FAULT_COLOR_INSUL    lv_color_hex(0x9C27B0)  /* 绝缘劣化 - 紫色 */
#define EW_FAULT_COLOR_CAP      lv_color_hex(0x00BCD4)  /* 电容老化 - 青色 */
#define EW_FAULT_COLOR_IGBT     lv_color_hex(0xE91E63)  /* IGBT故障 - 粉红 */
#define EW_FAULT_COLOR_BUS      lv_color_hex(0xFF5722)  /* 母线接地 - 深橙 */
#define EW_FAULT_COLOR_PWM      lv_color_hex(0x607D8B)  /* PWM异常 - 蓝灰 */

/* =========================
 * 辅助函数
 * ========================= */

static inline int32_t ew_theme_screen_w(void)
{
    return (int32_t)lv_display_get_horizontal_resolution(NULL);
}

static inline int32_t ew_theme_screen_h(void)
{
    return (int32_t)lv_display_get_vertical_resolution(NULL);
}

static inline bool ew_theme_is_compact(void)
{
    return (ew_theme_screen_w() <= 320);
}

/* 根据屏幕大小返回自适应间距 */
static inline int32_t ew_theme_adaptive_gap(int32_t normal_gap)
{
    return ew_theme_is_compact() ? (normal_gap * 2 / 3) : normal_gap;
}

/* 根据屏幕大小返回自适应圆角 */
static inline int32_t ew_theme_adaptive_radius(int32_t normal_radius)
{
    return ew_theme_is_compact() ? (normal_radius * 3 / 4) : normal_radius;
}

#ifdef __cplusplus
}
#endif

#endif /* EW_UI_THEME_REDESIGN_H */
