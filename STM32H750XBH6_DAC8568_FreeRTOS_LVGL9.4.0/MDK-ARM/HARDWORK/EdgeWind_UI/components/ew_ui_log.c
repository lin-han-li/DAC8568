/**
 * @file ew_ui_log.c
 * @brief UI Footer 日志实现（不调用 LVGL，跨任务安全）
 */

#include "ew_ui_log.h"

#include "ew_ui_theme.h"

#include "FreeRTOS.h"
#include "task.h"

#include <string.h>

static char s_log_buf[EW_UI_LOG_MAX_LEN] = {0};
static volatile bool s_dirty = false;

void ew_ui_log_set(const char *utf8_text)
{
    if (!utf8_text) utf8_text = "";

    taskENTER_CRITICAL();
    strncpy(s_log_buf, utf8_text, sizeof(s_log_buf) - 1);
    s_log_buf[sizeof(s_log_buf) - 1] = '\0';
    s_dirty = true;
    taskEXIT_CRITICAL();
}

bool ew_ui_log_take(char *out, size_t out_sz)
{
    if (!out || out_sz == 0) return false;

    bool has_new = false;

    taskENTER_CRITICAL();
    has_new = s_dirty;
    if (has_new) {
        strncpy(out, s_log_buf, out_sz - 1);
        out[out_sz - 1] = '\0';
        s_dirty = false;
    }
    taskEXIT_CRITICAL();

    return has_new;
}

