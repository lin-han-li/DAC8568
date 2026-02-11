/**
 * @file scr_main.h
 * @brief 主界面（参考根目录 主界面.HTML）
 */

#ifndef SCR_MAIN_H
#define SCR_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include <stdint.h>

/**
 * @brief 获取主界面 screen（首次调用会创建并缓存）
 */
lv_obj_t * ew_main_screen_get(void);

/**
 * @brief 显示主界面（带淡入动画）
 */
void ew_main_screen_show(void);

/**
 * @brief 打开占位页（复用一个占位 screen）
 * @param id 0~5 对应 6 个卡片
 */
void ew_main_placeholder_show(uint32_t id);

/**
 * @brief 设置 Footer 日志文本（LVGL 线程调用）
 */
void ew_main_set_footer_log(const char *utf8_text);

#ifdef __cplusplus
}
#endif

#endif /* SCR_MAIN_H */
