/**
 * @file edgewind_ui.h
 * @brief EdgeWind UI 主入口
 * 
 * 简化版：开机动画 + 单页主界面（6个按钮）
 */

#ifndef EDGEWIND_UI_H
#define EDGEWIND_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/*******************************************************************************
 * 函数声明
 ******************************************************************************/

/**
 * @brief 初始化 EdgeWind UI
 * @note  在 lv_init() 之后、进入主循环之前调用
 */
void edgewind_ui_init(void);

/**
 * @brief UI 周期刷新（在 LVGL 任务中调用）
 * @note  开机动画阶段可为空实现
 */
void edgewind_ui_refresh(void);

/**
 * @brief 检查开机动画是否完成
 * @return true 动画已完成
 */
bool edgewind_ui_boot_finished(void);

/**
 * @brief “进入系统”按钮点击入口（弱符号）
 * @note  在业务代码中实现同名函数即可接管。
 */
void edgewind_ui_on_enter_system(void);

/**
 * @brief 开机动画结束、"进入系统"按钮显示前回调（弱符号）
 * @note  典型用途：断电重连/自动上报预启动（此时还未进入系统 UI）。
 */
void edgewind_ui_on_before_enter_button(void);

/**
 * @brief 是否允许显示“进入系统”按钮（弱符号）
 * @return true 允许显示；false 暂不显示（例如后台同步未完成）
 */
bool edgewind_ui_can_show_enter_button(void);

/**
 * @brief 设置主界面 Footer 日志（跨任务可调用，不触碰 LVGL）
 * @note 业务层可在任意任务调用；LVGL 线程会在下一次刷新时取出并更新界面
 */
void edgewind_ui_log_set(const char *utf8_text);

#ifdef __cplusplus
}
#endif

#endif /* EDGEWIND_UI_H */
