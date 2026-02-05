/**
 * @file ew_ui_log.h
 * @brief UI Footer 日志（跨任务写入，LVGL 线程读取）
 */

#ifndef EW_UI_LOG_H
#define EW_UI_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

void ew_ui_log_set(const char *utf8_text);
bool ew_ui_log_take(char *out, size_t out_sz);

#ifdef __cplusplus
}
#endif

#endif /* EW_UI_LOG_H */

