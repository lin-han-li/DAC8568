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

#include "main.h"

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

/* Placeholder controls (2 rows x 3 cols) */
static uint8_t s_placeholder_fault_id = 0xFFu; /* 0..5 */
static uint32_t s_fault_duration_s[EW_FAULT_COUNT] = {10u, 10u, 10u, 10u, 10u, 10u};

static lv_obj_t * s_placeholder_grid = NULL;
static lv_obj_t * s_placeholder_duration_lbl = NULL;
static lv_obj_t * s_placeholder_minus_btn = NULL;
static lv_obj_t * s_placeholder_plus_btn = NULL;
static lv_obj_t * s_placeholder_trigger_btn = NULL;
static lv_obj_t * s_placeholder_stop_btn = NULL;
static lv_obj_t * s_placeholder_back2_btn = NULL;

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
static void placeholder_update_status(void);

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
    placeholder_update_status();
}

static lv_obj_t * create_footer(lv_obj_t * parent, lv_obj_t ** out_time_label, lv_obj_t ** out_log_label)
{
    const bool compact = (ew_ui_theme_screen_w() <= 240) || (ew_ui_theme_screen_h() <= 240);

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
    const bool compact = (ew_ui_theme_screen_w() <= 240) || (ew_ui_theme_screen_h() <= 240);

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
    const bool compact = (ew_ui_theme_screen_w() <= 240) || (ew_ui_theme_screen_h() <= 240);

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

typedef enum {
    PLACEHOLDER_ACT_MINUS = 1u,
    PLACEHOLDER_ACT_PLUS = 2u,
    PLACEHOLDER_ACT_TRIGGER = 3u,
    PLACEHOLDER_ACT_STOP = 4u,
    PLACEHOLDER_ACT_BACK = 5u
} placeholder_action_t;

static void placeholder_key_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;
    uint32_t key = lv_event_get_key(e);
    if (key == LV_KEY_ESC) {
        ew_main_screen_show();
    }
}

static void placeholder_update_duration_label(void)
{
    if (!s_placeholder_duration_lbl) return;
    uint32_t dur = 10u;
    if (s_placeholder_fault_id < EW_FAULT_COUNT) {
        dur = s_fault_duration_s[s_placeholder_fault_id];
    }
    char buf[32];
    (void)snprintf(buf, sizeof(buf), "时长:%lus", (unsigned long)dur);
    lv_label_set_text(s_placeholder_duration_lbl, buf);
}

static void placeholder_update_status(void)
{
    if (!s_placeholder_status_lbl) {
        return;
    }

    uint32_t ready_mask = 0u;
    uint8_t active_fault_id_0_5 = 0xFFu;
    uint32_t remaining_s = 0u;
    DAC_FaultBurst_GetUiState(&ready_mask, &active_fault_id_0_5, &remaining_s);

    const bool normal_ready = ((ready_mask & 0x1u) != 0u);
    const uint32_t fault_bit = (s_placeholder_fault_id < EW_FAULT_COUNT)
                                   ? (1u << (uint32_t)(s_placeholder_fault_id + 1u))
                                   : 0u;
    const bool fault_ready = ((fault_bit != 0u) && ((ready_mask & fault_bit) != 0u));

    const char *out_cn = "正常";
    if (active_fault_id_0_5 < EW_FAULT_COUNT) {
        const ew_fault_config_t *cfg = ew_fault_get((uint32_t)active_fault_id_0_5);
        out_cn = (cfg && cfg->name_cn) ? cfg->name_cn : "故障";
    }

    char buf[96];
    if (active_fault_id_0_5 < EW_FAULT_COUNT && remaining_s > 0u) {
        (void)snprintf(buf, sizeof(buf),
                       "输出:%s 剩余:%lus\nnormal:%s 当前:%s",
                       out_cn,
                       (unsigned long)remaining_s,
                       normal_ready ? "就绪" : "未就绪",
                       fault_ready ? "就绪" : "未就绪");
    } else {
        (void)snprintf(buf, sizeof(buf),
                       "输出:%s\nnormal:%s 当前:%s",
                       out_cn,
                       normal_ready ? "就绪" : "未就绪",
                       fault_ready ? "就绪" : "未就绪");
    }
    lv_label_set_text(s_placeholder_status_lbl, buf);

    if (s_placeholder_trigger_btn) {
        const bool can_trigger = normal_ready && fault_ready;
        if (can_trigger) {
            lv_obj_clear_state(s_placeholder_trigger_btn, LV_STATE_DISABLED);
        } else {
            lv_obj_add_state(s_placeholder_trigger_btn, LV_STATE_DISABLED);
        }
    }

    if (s_placeholder_stop_btn) {
        const bool can_stop = (active_fault_id_0_5 < EW_FAULT_COUNT);
        if (can_stop) {
            lv_obj_clear_state(s_placeholder_stop_btn, LV_STATE_DISABLED);
        } else {
            lv_obj_add_state(s_placeholder_stop_btn, LV_STATE_DISABLED);
        }
    }
}

static void placeholder_action_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    const placeholder_action_t act = (placeholder_action_t)(uintptr_t)lv_event_get_user_data(e);
    if (s_placeholder_fault_id >= EW_FAULT_COUNT) {
        if (act == PLACEHOLDER_ACT_BACK) {
            ew_main_screen_show();
        }
        return;
    }

    uint32_t dur = s_fault_duration_s[s_placeholder_fault_id];
    if (dur < 1u) dur = 1u;
    if (dur > 3600u) dur = 3600u;

    switch (act) {
    case PLACEHOLDER_ACT_MINUS:
        if (dur > 1u) dur--;
        s_fault_duration_s[s_placeholder_fault_id] = dur;
        placeholder_update_duration_label();
        break;
    case PLACEHOLDER_ACT_PLUS:
        if (dur < 3600u) dur++;
        s_fault_duration_s[s_placeholder_fault_id] = dur;
        placeholder_update_duration_label();
        break;
    case PLACEHOLDER_ACT_TRIGGER: {
        bool ok = DAC_FaultBurst_Trigger((uint32_t)s_placeholder_fault_id, dur);
        if (!ok && s_placeholder_status_lbl) {
            lv_label_set_text(s_placeholder_status_lbl, "触发失败(波形未就绪)");
        }
        break;
    }
    case PLACEHOLDER_ACT_STOP:
        DAC_FaultBurst_Stop();
        break;
    case PLACEHOLDER_ACT_BACK:
        ew_main_screen_show();
        return;
    default:
        break;
    }

    placeholder_update_status();
}

static lv_obj_t * placeholder_make_btn(lv_obj_t * parent, const char *text, const lv_font_t *font,
                                       int32_t w, int32_t h, placeholder_action_t act)
{
    lv_obj_t * btn = lv_button_create(parent);
    lv_obj_remove_style_all(btn);
    lv_obj_add_style(btn, &ew_style_card_main, 0);
    lv_obj_add_style(btn, &ew_style_card_pressed_base, LV_STATE_PRESSED);
    lv_obj_set_size(btn, w, h);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_event_cb(btn, placeholder_action_event_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)act);
    lv_obj_add_event_cb(btn, placeholder_key_event_cb, LV_EVENT_KEY, NULL);

    /* Focus frame for keypad/encoder navigation */
    lv_obj_set_style_border_width(btn, 2, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_border_color(btn, EW_UI_COLOR_THEME, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(btn, 1, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_outline_pad(btn, 1, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_outline_color(btn, EW_UI_COLOR_THEME, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_outline_opa(btn, LV_OPA_70, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_border_width(btn, 2, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_color(btn, EW_UI_COLOR_THEME, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(btn, 1, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(btn, 1, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(btn, EW_UI_COLOR_THEME, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_opa(btn, LV_OPA_80, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

    lv_obj_t * lbl = lv_label_create(btn);
    lv_obj_set_style_text_font(lbl, font, 0);
    lv_obj_set_style_text_color(lbl, EW_UI_COLOR_TEXT_DARK, 0);
    lv_label_set_text(lbl, text ? text : "");
    lv_obj_add_flag(lbl, LV_OBJ_FLAG_EVENT_BUBBLE);
    return btn;
}

static lv_obj_t * placeholder_make_cell(lv_obj_t * parent, int32_t w, int32_t h)
{
    lv_obj_t * cell = lv_obj_create(parent);
    lv_obj_remove_style_all(cell);
    lv_obj_add_style(cell, &ew_style_card_main, 0);
    lv_obj_set_size(cell, w, h);
    lv_obj_clear_flag(cell, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(cell, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cell, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    return cell;
}

static void placeholder_build_once(void)
{
    const bool compact = (ew_ui_theme_screen_w() <= 240) || (ew_ui_theme_screen_h() <= 240);
    const int32_t screen_h = ew_ui_theme_screen_h();
    const int32_t body_pad = compact ? 4 : EW_UI_BODY_PAD;

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
    if (s_placeholder_back_btn) {
        lv_obj_add_event_cb(s_placeholder_back_btn, placeholder_key_event_cb, LV_EVENT_KEY, NULL);
    }

    lv_obj_t * body = lv_obj_create(s_placeholder_scr);
    lv_obj_remove_style_all(body);
    lv_obj_set_style_bg_opa(body, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(body, 0, 0);
    lv_obj_set_style_pad_all(body, body_pad, 0);
    /* In flex-column parent width defaults to content; force full width for placeholder layout. */
    lv_obj_set_width(body, LV_PCT(100));
    lv_obj_set_flex_grow(body, 1);
    lv_obj_clear_flag(body, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(body, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(body, compact ? 4 : 10, 0);

    s_placeholder_icon_lbl = lv_label_create(body);
    lv_obj_set_style_text_font(s_placeholder_icon_lbl, compact ? &lv_font_montserrat_20 : &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(s_placeholder_icon_lbl, EW_UI_COLOR_THEME, 0);
    lv_label_set_text(s_placeholder_icon_lbl, LV_SYMBOL_DUMMY);

    s_placeholder_status_lbl = lv_label_create(body);
    lv_obj_add_style(s_placeholder_status_lbl, &ew_style_text_cn, 0);
    if (compact) {
        lv_obj_set_style_text_font(s_placeholder_status_lbl, EW_FONT_CN_12, 0);
    }
    lv_obj_set_width(s_placeholder_status_lbl, LV_PCT(100));
    lv_obj_set_style_text_align(s_placeholder_status_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(s_placeholder_status_lbl, "加载中...");

    s_placeholder_desc_lbl = lv_label_create(body);
    lv_obj_add_style(s_placeholder_desc_lbl, &ew_style_text_en, 0);
    if (compact) {
        lv_obj_add_flag(s_placeholder_desc_lbl, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_set_width(s_placeholder_desc_lbl, LV_PCT(100));
    lv_obj_set_style_text_align(s_placeholder_desc_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(s_placeholder_desc_lbl, "");

    /* Push action grid to the bottom of body */
    lv_obj_t * spacer = lv_obj_create(body);
    lv_obj_remove_style_all(spacer);
    lv_obj_clear_flag(spacer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, 0);
    lv_obj_set_size(spacer, 1, 1);
    lv_obj_set_flex_grow(spacer, 1);

    /* 2 rows x 3 cols action grid */
    const int32_t screen_w = ew_ui_theme_screen_w();
    const int32_t pad = body_pad;
    const int32_t grid_gap = compact ? 6 : 8;
    int32_t avail_w = screen_w - 2 * pad;
    if (avail_w < 0) avail_w = 0;
    int32_t btn_w = (avail_w - 2 * grid_gap) / 3;
    if (btn_w < 56) btn_w = 56;
    if ((3 * btn_w + 2 * grid_gap) > avail_w) {
        btn_w = (avail_w >= 2 * grid_gap) ? ((avail_w - 2 * grid_gap) / 3) : (avail_w / 3);
    }
    int32_t btn_h = compact ? 30 : 40;
    if (compact) {
        int32_t body_h = screen_h - EW_UI_HEADER_H - EW_UI_FOOTER_H;
        if (body_h < 80) body_h = 80;
        const int32_t top_reserved = 44; /* icon + status + row gaps */
        int32_t max_grid_h = body_h - 2 * body_pad - top_reserved;
        if (max_grid_h < 2 * 24 + grid_gap) {
            max_grid_h = 2 * 24 + grid_gap;
        }
        int32_t fit_btn_h = (max_grid_h - grid_gap) / 2;
        if (fit_btn_h < 24) fit_btn_h = 24;
        if (fit_btn_h < btn_h) btn_h = fit_btn_h;
    }

    s_placeholder_grid = lv_obj_create(body);
    lv_obj_remove_style_all(s_placeholder_grid);
    lv_obj_clear_flag(s_placeholder_grid, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(s_placeholder_grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_row(s_placeholder_grid, grid_gap, 0);
    lv_obj_set_style_pad_column(s_placeholder_grid, grid_gap, 0);
    lv_obj_set_size(s_placeholder_grid, LV_PCT(100), 2 * btn_h + grid_gap);
    lv_obj_set_flex_flow(s_placeholder_grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(s_placeholder_grid, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    const lv_font_t * btn_font = compact ? EW_FONT_CN_12 : EW_FONT_CN_14;
    s_placeholder_minus_btn = placeholder_make_btn(s_placeholder_grid, "-1s", btn_font, btn_w, btn_h, PLACEHOLDER_ACT_MINUS);

    lv_obj_t * dur_cell = placeholder_make_cell(s_placeholder_grid, btn_w, btn_h);
    s_placeholder_duration_lbl = lv_label_create(dur_cell);
    lv_obj_set_style_text_font(s_placeholder_duration_lbl, btn_font, 0);
    lv_obj_set_style_text_color(s_placeholder_duration_lbl, EW_UI_COLOR_TEXT_DARK, 0);
    lv_label_set_text(s_placeholder_duration_lbl, "");
    placeholder_update_duration_label();

    s_placeholder_plus_btn = placeholder_make_btn(s_placeholder_grid, "+1s", btn_font, btn_w, btn_h, PLACEHOLDER_ACT_PLUS);
    s_placeholder_trigger_btn = placeholder_make_btn(s_placeholder_grid, "触发", btn_font, btn_w, btn_h, PLACEHOLDER_ACT_TRIGGER);
    s_placeholder_stop_btn = placeholder_make_btn(s_placeholder_grid, "停止", btn_font, btn_w, btn_h, PLACEHOLDER_ACT_STOP);
    s_placeholder_back2_btn = placeholder_make_btn(s_placeholder_grid, "返回", btn_font, btn_w, btn_h, PLACEHOLDER_ACT_BACK);

    if (s_placeholder_group) {
        if (s_placeholder_minus_btn) lv_group_add_obj(s_placeholder_group, s_placeholder_minus_btn);
        if (s_placeholder_plus_btn) lv_group_add_obj(s_placeholder_group, s_placeholder_plus_btn);
        if (s_placeholder_trigger_btn) lv_group_add_obj(s_placeholder_group, s_placeholder_trigger_btn);
        if (s_placeholder_stop_btn) lv_group_add_obj(s_placeholder_group, s_placeholder_stop_btn);
        if (s_placeholder_back2_btn) lv_group_add_obj(s_placeholder_group, s_placeholder_back2_btn);
    }

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
    bool compact = false;
    lv_obj_t *first_card = NULL;

    int32_t body_h = screen_h - EW_UI_HEADER_H - EW_UI_FOOTER_H;
    if (body_h < 0) body_h = 0;

    /* Force 3 columns x 2 rows (2行3列). */
    int32_t card_w = (screen_w - 2 * pad - 2 * gap) / 3;
    int32_t card_h = (body_h - gap) / 2;
    /* Use compact card content when 3x2 cards are small (e.g., 240x320, 320x240, 320x480). */
    compact = (card_w < 120) || (card_h < 64);
    if (compact) {
        if (card_w < 56) card_w = 56;
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

    if (id >= EW_FAULT_COUNT) {
        id = 0u;
    }
    s_placeholder_fault_id = (uint8_t)id;

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

    placeholder_update_duration_label();
    placeholder_update_status();
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
