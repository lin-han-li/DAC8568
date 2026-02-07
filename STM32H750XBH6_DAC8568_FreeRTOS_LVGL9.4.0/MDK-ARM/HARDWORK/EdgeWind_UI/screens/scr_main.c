/**
 * @file scr_main.c
 * @brief 主界面（2行3列卡片布局 + 键控选中框）
 */

#include "scr_main.h"

#include "../components/ew_ui_anim.h"
#include "../components/ew_ui_fault_model.h"
#include "../components/ew_ui_styles.h"
#include "../components/ew_ui_theme.h"
#include "lv_port_indev.h"

#include "../fonts/ew_fonts.h"

#include "rtc.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define EW_UI_DEFAULT_LOG  "[边缘节点]：正在分析母线谐波数据..."

/* Cached screens */
static lv_obj_t * s_main_scr = NULL;
static lv_obj_t * s_placeholder_scr = NULL;
static lv_obj_t * s_main_first_card = NULL;
static lv_obj_t * s_placeholder_back_btn = NULL;
static lv_group_t * s_main_group = NULL;
static lv_group_t * s_placeholder_group = NULL;

/* Footer labels */
static lv_obj_t * s_main_time_lbl = NULL;
static lv_obj_t * s_main_log_lbl = NULL;
static lv_obj_t * s_placeholder_time_lbl = NULL;
static lv_obj_t * s_placeholder_log_lbl = NULL;

/* Placeholder widgets */
static lv_obj_t * s_placeholder_title_lbl = NULL;
static lv_obj_t * s_placeholder_icon_lbl = NULL;
static lv_obj_t * s_placeholder_desc_lbl = NULL;
static lv_obj_t * s_placeholder_status_lbl = NULL;

/* Timer for footer clock */
static lv_timer_t * s_time_timer = NULL;
static uint16_t s_last_minute = 0xFFFFu;
static uint16_t s_last_year = 0xFFFFu;
static uint8_t s_last_month = 0xFFu;
static uint8_t s_last_day = 0xFFu;
static uint8_t s_last_hour = 0xFFu;

static char s_last_log[EW_UI_LOG_MAX_LEN] = EW_UI_DEFAULT_LOG;

static void footer_time_update(void);
static void time_timer_cb(lv_timer_t *timer);

static lv_obj_t * build_main_header(lv_obj_t * parent);
static lv_obj_t * build_placeholder_header(lv_obj_t * parent);
static lv_obj_t * create_footer(lv_obj_t * parent, lv_obj_t ** out_time_label, lv_obj_t ** out_log_label);

static void placeholder_build_once(void);

static void placeholder_back_event_cb(lv_event_t *e);
static void card_event_cb(lv_event_t *e);

static void footer_time_update(void)
{
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    if (s_last_minute == sTime.Minutes &&
        s_last_hour == sTime.Hours &&
        s_last_day == sDate.Date &&
        s_last_month == sDate.Month &&
        s_last_year == sDate.Year) {
        return;
    }

    s_last_minute = sTime.Minutes;
    s_last_hour = sTime.Hours;
    s_last_day = sDate.Date;
    s_last_month = sDate.Month;
    s_last_year = sDate.Year;

    char buf[32];
    const uint16_t year = (uint16_t)(2000u + (uint16_t)sDate.Year);
    (void)snprintf(buf, sizeof(buf), "%04u-%02u-%02u %02u:%02u",
                   (unsigned)year,
                   (unsigned)sDate.Month,
                   (unsigned)sDate.Date,
                   (unsigned)sTime.Hours,
                   (unsigned)sTime.Minutes);

    if (s_main_time_lbl) lv_label_set_text(s_main_time_lbl, buf);
    if (s_placeholder_time_lbl) lv_label_set_text(s_placeholder_time_lbl, buf);
}

static void time_timer_cb(lv_timer_t *timer)
{
    LV_UNUSED(timer);
    footer_time_update();
}

static lv_obj_t * create_footer(lv_obj_t * parent, lv_obj_t ** out_time_label, lv_obj_t ** out_log_label)
{
    const bool compact = (ew_ui_theme_screen_w() <= 240);

    lv_obj_t * footer = lv_obj_create(parent);
    lv_obj_remove_style_all(footer);
    lv_obj_add_style(footer, &ew_style_footer, 0);
    lv_obj_set_size(footer, LV_PCT(100), EW_UI_FOOTER_H);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(footer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(footer, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(footer, compact ? 6 : 12, 0);

    lv_obj_t * left = lv_label_create(footer);
    lv_obj_add_style(left, &ew_style_text_footer, 0);
    lv_label_set_text(left, compact ? "项目单位：研发组" : "项目单位：风电场直流系统研发组");
    lv_label_set_long_mode(left, LV_LABEL_LONG_DOT);
    lv_obj_set_width(left, compact ? 96 : 250);

    lv_obj_t * log = lv_label_create(footer);
    lv_obj_add_style(log, &ew_style_text_footer, 0);
    lv_obj_set_flex_grow(log, 1);
    lv_label_set_long_mode(log, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_label_set_text(log, (s_last_log[0] != '\0') ? s_last_log : EW_UI_DEFAULT_LOG);

    lv_obj_t * right = lv_label_create(footer);
    lv_obj_add_style(right, &ew_style_text_footer, 0);
    lv_obj_set_style_text_color(right, EW_UI_COLOR_TEXT_DARK, 0);
    lv_label_set_text(right, "");
    lv_obj_set_width(right, compact ? 86 : 160);
    lv_label_set_long_mode(right, LV_LABEL_LONG_CLIP);

    if (out_time_label) *out_time_label = right;
    if (out_log_label) *out_log_label = log;
    return footer;
}

static lv_obj_t * build_main_header(lv_obj_t * parent)
{
    const bool compact = (ew_ui_theme_screen_w() <= 240);

    lv_obj_t * header = lv_obj_create(parent);
    lv_obj_remove_style_all(header);
    lv_obj_add_style(header, &ew_style_header, 0);
    lv_obj_set_size(header, LV_PCT(100), EW_UI_HEADER_H);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t * title = lv_label_create(header);
    lv_obj_add_style(title, &ew_style_text_title, 0);
    lv_label_set_text(title, compact ? "直流故障诊断平台" : "直流系统故障模拟诊断平台");

    lv_obj_t * badge = lv_obj_create(header);
    lv_obj_remove_style_all(badge);
    lv_obj_add_style(badge, &ew_style_badge, 0);
    lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(badge, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(badge, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t * dot = lv_obj_create(badge);
    lv_obj_remove_style_all(dot);
    lv_obj_set_size(dot, 10, 10);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dot, EW_UI_COLOR_SAFE, 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);

    lv_obj_t * status = lv_label_create(badge);
    lv_obj_add_style(status, &ew_style_badge_text, 0);
    lv_label_set_text(status, "边缘节点已连接");

    if (compact) {
        lv_obj_add_flag(badge, LV_OBJ_FLAG_HIDDEN);
    }

    return header;
}

static lv_obj_t * build_placeholder_header(lv_obj_t * parent)
{
    const bool compact = (ew_ui_theme_screen_w() <= 240);

    lv_obj_t * header = lv_obj_create(parent);
    lv_obj_remove_style_all(header);
    lv_obj_add_style(header, &ew_style_header, 0);
    lv_obj_set_size(header, LV_PCT(100), EW_UI_HEADER_H);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(header, compact ? 6 : 12, 0);

    lv_obj_t * back_btn = lv_button_create(header);
    lv_obj_remove_style_all(back_btn);
    lv_obj_add_style(back_btn, &ew_style_back_btn, 0);
    lv_obj_add_style(back_btn, &ew_style_back_btn_pressed, LV_STATE_PRESSED);
    lv_obj_clear_flag(back_btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(back_btn, placeholder_back_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_flex_flow(back_btn, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(back_btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(back_btn, compact ? 4 : 6, 0);

    lv_obj_t * back_icn = lv_label_create(back_btn);
    lv_obj_set_style_text_font(back_icn, compact ? &lv_font_montserrat_12 : &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(back_icn, EW_UI_COLOR_THEME, 0);
    lv_label_set_text(back_icn, LV_SYMBOL_LEFT);
    lv_obj_add_flag(back_icn, LV_OBJ_FLAG_EVENT_BUBBLE);

    lv_obj_t * back_txt = lv_label_create(back_btn);
    lv_obj_set_style_text_font(back_txt, compact ? EW_FONT_CN_12 : EW_FONT_CN_14, 0);
    lv_obj_set_style_text_color(back_txt, EW_UI_COLOR_THEME, 0);
    lv_label_set_text(back_txt, "返回");
    lv_obj_add_flag(back_txt, LV_OBJ_FLAG_EVENT_BUBBLE);

    lv_obj_update_layout(back_btn);
    const int32_t bw = lv_obj_get_width(back_btn);
    const int32_t bh = lv_obj_get_height(back_btn);
    lv_obj_set_style_transform_pivot_x(back_btn, bw / 2, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_transform_pivot_y(back_btn, bh / 2, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_width(back_btn, 2, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_border_color(back_btn, EW_UI_COLOR_THEME, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(back_btn, 1, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_outline_pad(back_btn, 1, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_outline_color(back_btn, EW_UI_COLOR_THEME, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_outline_opa(back_btn, LV_OPA_70, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_bg_color(back_btn, EW_UI_COLOR_THEME_BG, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_bg_opa(back_btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_border_width(back_btn, 2, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_color(back_btn, EW_UI_COLOR_THEME, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(back_btn, 1, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(back_btn, 1, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(back_btn, EW_UI_COLOR_THEME, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_opa(back_btn, LV_OPA_80, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_bg_color(back_btn, EW_UI_COLOR_THEME_BG, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_bg_opa(back_btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    s_placeholder_back_btn = back_btn;

    lv_obj_t * title = lv_label_create(header);
    lv_obj_add_style(title, &ew_style_text_title, 0);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(title, "");
    lv_obj_set_flex_grow(title, 1);
    s_placeholder_title_lbl = title;

    lv_obj_t * spacer = lv_obj_create(header);
    lv_obj_remove_style_all(spacer);
    lv_obj_clear_flag(spacer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(spacer, bw, 1);
    lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, 0);

    return header;
}

static void placeholder_build_once(void)
{
    const bool compact = (ew_ui_theme_screen_w() <= 240);

    ew_ui_styles_init_once();
    if (!s_main_group) {
        s_main_group = lv_group_create();
        lv_group_set_wrap(s_main_group, true);
    }
    if (!s_placeholder_group) {
        s_placeholder_group = lv_group_create();
        lv_group_set_wrap(s_placeholder_group, false);
    }
    if (s_placeholder_scr) return;

    s_placeholder_scr = lv_obj_create(NULL);
    lv_obj_remove_style_all(s_placeholder_scr);
    lv_obj_add_style(s_placeholder_scr, &ew_style_screen_bg, 0);
    lv_obj_clear_flag(s_placeholder_scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(s_placeholder_scr, LV_FLEX_FLOW_COLUMN);

    (void)build_placeholder_header(s_placeholder_scr);
    if (s_placeholder_group && s_placeholder_back_btn) {
        lv_group_add_obj(s_placeholder_group, s_placeholder_back_btn);
    }

    lv_obj_t * body = lv_obj_create(s_placeholder_scr);
    lv_obj_remove_style_all(body);
    lv_obj_set_style_bg_opa(body, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(body, 0, 0);
    lv_obj_set_style_pad_all(body, EW_UI_BODY_PAD, 0);
    lv_obj_set_flex_grow(body, 1);
    lv_obj_clear_flag(body, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(body, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(body, compact ? 6 : 10, 0);

    s_placeholder_icon_lbl = lv_label_create(body);
    lv_obj_set_style_text_font(s_placeholder_icon_lbl, compact ? &lv_font_montserrat_28 : &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(s_placeholder_icon_lbl, EW_UI_COLOR_THEME, 0);
    lv_label_set_text(s_placeholder_icon_lbl, LV_SYMBOL_DUMMY);

    s_placeholder_status_lbl = lv_label_create(body);
    lv_obj_add_style(s_placeholder_status_lbl, &ew_style_text_cn, 0);
    lv_label_set_text(s_placeholder_status_lbl, "加载中...");

    s_placeholder_desc_lbl = lv_label_create(body);
    lv_obj_add_style(s_placeholder_desc_lbl, &ew_style_text_en, 0);
    lv_label_set_text(s_placeholder_desc_lbl, "");

    (void)create_footer(s_placeholder_scr, &s_placeholder_time_lbl, &s_placeholder_log_lbl);

    if (!s_time_timer) {
        s_time_timer = lv_timer_create(time_timer_cb, 1000, NULL);
    }

    footer_time_update();
}

lv_obj_t * ew_main_screen_get(void)
{
    ew_ui_styles_init_once();
    if (s_main_scr) return s_main_scr;

    if (!s_main_group) {
        s_main_group = lv_group_create();
        lv_group_set_wrap(s_main_group, true);
    }
    if (!s_placeholder_group) {
        s_placeholder_group = lv_group_create();
        lv_group_set_wrap(s_placeholder_group, false);
    }

    s_main_scr = lv_obj_create(NULL);
    lv_obj_remove_style_all(s_main_scr);
    lv_obj_add_style(s_main_scr, &ew_style_screen_bg, 0);
    lv_obj_clear_flag(s_main_scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(s_main_scr, LV_FLEX_FLOW_COLUMN);

    (void)build_main_header(s_main_scr);

    lv_obj_t * body = lv_obj_create(s_main_scr);
    lv_obj_remove_style_all(body);
    lv_obj_set_style_bg_opa(body, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(body, 0, 0);
    lv_obj_set_style_pad_all(body, EW_UI_BODY_PAD, 0);
    lv_obj_set_style_pad_gap(body, EW_UI_BODY_GAP, 0);
    /* In flex column parent, width defaults to LV_SIZE_CONTENT; force full width for 3x2 wrap layout */
    lv_obj_set_width(body, LV_PCT(100));
    lv_obj_set_flex_grow(body, 1);
    lv_obj_clear_flag(body, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(body, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    const int32_t screen_w = ew_ui_theme_screen_w();
    const int32_t screen_h = ew_ui_theme_screen_h();
    const int32_t pad = EW_UI_BODY_PAD;
    const int32_t gap = EW_UI_BODY_GAP;
    /* 320x240 landscape should use compact card content to fit 3x2. */
    const bool compact = ((screen_w <= 320) && (screen_h <= 240));
    lv_obj_t *first_card = NULL;

    int32_t body_h = screen_h - EW_UI_HEADER_H - EW_UI_FOOTER_H;
    if (body_h < 0) body_h = 0;

    /* Force 3 columns x 2 rows (2行3列). */
    int32_t card_w = (screen_w - 2 * pad - 2 * gap) / 3;
    int32_t card_h = (body_h - gap) / 2;
    if (compact) {
        if (card_w < 72) card_w = 72;
        if (card_h < 44) card_h = 44;
    } else {
        if (card_w < 120) card_w = 120;
        if (card_h < 64) card_h = 64;
    }

    for (uint32_t i = 0; i < EW_FAULT_COUNT; i++) {
        const ew_fault_config_t * cfg = ew_fault_get(i);
        if (!cfg) continue;

        lv_obj_t * card = lv_button_create(body);
        lv_obj_remove_style_all(card);
        lv_obj_add_style(card, &ew_style_card_main, 0);
        lv_obj_add_style(card, &ew_style_card_pressed_base, LV_STATE_PRESSED);
        lv_obj_set_size(card, card_w, card_h);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(card, card_event_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)i);

        if (first_card == NULL) {
            first_card = card;
        }
        lv_group_add_obj(s_main_group, card);

        /* Per-card pressed feedback (theme color) */
        lv_obj_set_style_border_color(card, cfg->theme_color, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_shadow_color(card, cfg->theme_color, LV_PART_MAIN | LV_STATE_PRESSED);

        /* Focused selection frame for keypad navigation visibility */
        lv_obj_set_style_border_width(card, 3, LV_PART_MAIN | LV_STATE_FOCUSED);
        lv_obj_set_style_border_color(card, cfg->theme_color, LV_PART_MAIN | LV_STATE_FOCUSED);
        lv_obj_set_style_outline_width(card, 2, LV_PART_MAIN | LV_STATE_FOCUSED);
        lv_obj_set_style_outline_pad(card, 1, LV_PART_MAIN | LV_STATE_FOCUSED);
        lv_obj_set_style_outline_color(card, cfg->theme_color, LV_PART_MAIN | LV_STATE_FOCUSED);
        lv_obj_set_style_outline_opa(card, LV_OPA_70, LV_PART_MAIN | LV_STATE_FOCUSED);
        lv_obj_set_style_border_width(card, 3, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
        lv_obj_set_style_border_color(card, cfg->theme_color, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_width(card, 2, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_pad(card, 1, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_color(card, cfg->theme_color, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_opa(card, LV_OPA_80, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

        /* Keep pressed scale from center */
        lv_obj_set_style_transform_pivot_x(card, card_w / 2, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_transform_pivot_y(card, card_h / 2, LV_PART_MAIN | LV_STATE_PRESSED);

        /* Staggered fade-in */
        ew_ui_anim_fade_in(card, i * 100u, 500u);

        /* Decorative color bar */
        lv_obj_t * bar = lv_obj_create(card);
        lv_obj_remove_style_all(bar);
        lv_obj_set_size(bar, compact ? 3 : 6, compact ? 18 : 40);
        lv_obj_set_style_bg_color(bar, cfg->theme_color, 0);
        lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(bar, 3, 0);
        lv_obj_add_flag(bar, LV_OBJ_FLAG_IGNORE_LAYOUT | LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_clear_flag(bar, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_pos(bar, 0, compact ? 4 : 14);

        /* Content container */
        lv_obj_t * cont = lv_obj_create(card);
        lv_obj_remove_style_all(cont);
        lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(cont, 0, 0);
        lv_obj_set_style_pad_all(cont, compact ? 3 : 16, 0);
        lv_obj_set_style_pad_left(cont, compact ? 7 : 24, 0);
        lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_row(cont, compact ? 0 : 6, 0);
        lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(cont, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_clear_flag(cont, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_t * icn = lv_label_create(cont);
        lv_obj_set_style_text_font(icn, compact ? &lv_font_montserrat_14 : &lv_font_montserrat_32, 0);
        lv_obj_set_style_text_color(icn, cfg->theme_color, 0);
        lv_label_set_text(icn, cfg->icon);
        lv_obj_add_flag(icn, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_clear_flag(icn, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_t * cn = lv_label_create(cont);
        lv_obj_add_style(cn, &ew_style_text_cn, 0);
        if (compact) {
            lv_obj_set_style_text_font(cn, EW_FONT_CN_12, 0);
        }
        lv_label_set_text(cn, cfg->name_cn);
        lv_obj_add_flag(cn, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_clear_flag(cn, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_t * en = lv_label_create(cont);
        lv_obj_add_style(en, &ew_style_text_en, 0);
        if (compact) {
            lv_obj_add_flag(en, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_label_set_text(en, cfg->name_en);
        }
        lv_obj_add_flag(en, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_clear_flag(en, LV_OBJ_FLAG_CLICKABLE);
    }

    if (first_card) {
        s_main_first_card = first_card;
    }

    (void)create_footer(s_main_scr, &s_main_time_lbl, &s_main_log_lbl);

    if (!s_time_timer) {
        s_time_timer = lv_timer_create(time_timer_cb, 1000, NULL);
    }
    footer_time_update();

    return s_main_scr;
}

void ew_main_screen_show(void)
{
    (void)ew_main_screen_get();
    lv_port_indev_set_group(s_main_group);
    if (s_main_first_card) {
        lv_group_focus_obj(s_main_first_card);
    }
    lv_screen_load_anim(s_main_scr, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, false);
    footer_time_update();
}

void ew_main_placeholder_show(uint32_t id)
{
    placeholder_build_once();
    lv_port_indev_set_group(s_placeholder_group);
    if (s_placeholder_back_btn) {
        lv_group_focus_obj(s_placeholder_back_btn);
    }

    const ew_fault_config_t * cfg = ew_fault_get(id);
    if (!cfg) cfg = ew_fault_get(0);
    if (!cfg) return;

    if (s_placeholder_title_lbl) lv_label_set_text(s_placeholder_title_lbl, cfg->name_cn);
    if (s_placeholder_icon_lbl) {
        lv_label_set_text(s_placeholder_icon_lbl, cfg->icon);
        lv_obj_set_style_text_color(s_placeholder_icon_lbl, cfg->theme_color, 0);
    }
    if (s_placeholder_desc_lbl) lv_label_set_text(s_placeholder_desc_lbl, cfg->name_en);
    if (s_placeholder_status_lbl) lv_label_set_text(s_placeholder_status_lbl, "加载中...");

    lv_screen_load_anim(s_placeholder_scr, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, false);
    footer_time_update();
}

void ew_main_set_footer_log(const char *utf8_text)
{
    if (!utf8_text) utf8_text = "";

    if (utf8_text[0] == '\0') {
        strncpy(s_last_log, EW_UI_DEFAULT_LOG, sizeof(s_last_log) - 1);
        s_last_log[sizeof(s_last_log) - 1] = '\0';
    } else {
        strncpy(s_last_log, utf8_text, sizeof(s_last_log) - 1);
        s_last_log[sizeof(s_last_log) - 1] = '\0';
    }

    if (s_main_log_lbl) lv_label_set_text(s_main_log_lbl, s_last_log);
    if (s_placeholder_log_lbl) lv_label_set_text(s_placeholder_log_lbl, s_last_log);
}

static void placeholder_back_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    ew_main_screen_show();
}

static void card_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    const uint32_t id = (uint32_t)(uintptr_t)lv_event_get_user_data(e);
    ew_main_placeholder_show(id);
}
