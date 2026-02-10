/**
 * @file edgewind_ui.c
 * @brief EdgeWind UI 主入口实现
 * 
 * 简化版：开机动画 + 单页主界面（6个按钮）
 */

#include "edgewind_ui.h"
#include "screens/scr_boot_anim.h"
#include "screens/scr_main.h"
#include "components/ew_ui_log.h"

/*******************************************************************************
 * 内部变量
 ******************************************************************************/

static bool ui_initialized = false;

/*******************************************************************************
 * 弱回调：进入系统按钮点击入口
 ******************************************************************************/
#if defined(__GNUC__) || defined(__clang__)
#define EW_WEAK __attribute__((weak))
#else
#define EW_WEAK __weak
#endif

EW_WEAK void edgewind_ui_on_enter_system(void)
{
    ew_main_screen_show();
}

/* 弱回调：开机动画结束、进入系统按钮显示前 */
EW_WEAK void edgewind_ui_on_before_enter_button(void)
{
}

EW_WEAK bool edgewind_ui_can_show_enter_button(void)
{
    return true;
}

/*******************************************************************************
 * 公共函数实现
 ******************************************************************************/

void edgewind_ui_init(void)
{
    if (ui_initialized) return;
    
    /* 创建并加载开机动画界面 */
    ew_boot_anim_create();
    lv_scr_load(ew_boot_anim.screen);

    /* Default footer log (can be updated by business tasks later) */
    edgewind_ui_log_set("[边缘节点]：正在分析母线谐波数据...");

    ui_initialized = true;
}

void edgewind_ui_refresh(void)
{
    char buf[160];

    /* Pull newest log text (if any) and update UI in LVGL thread */
    if (ew_ui_log_take(buf, sizeof(buf))) {
        ew_main_set_footer_log(buf);
    }
}

bool edgewind_ui_boot_finished(void)
{
    return ew_boot_anim_is_finished();
}

void edgewind_ui_log_set(const char *utf8_text)
{
    ew_ui_log_set(utf8_text);
}
