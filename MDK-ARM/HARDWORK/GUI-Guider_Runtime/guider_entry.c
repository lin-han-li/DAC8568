/**
 * @file guider_entry.c
 * @brief GUI Guider 入口：开机动画后进入 Main_1/2/3 三页可滑动主界面
 */

#include "../EdgeWind_UI/edgewind_ui.h"
#include "gui_assets.h"
#include "src/generated/gui_guider.h"

lv_ui guider_ui;

static bool guider_initialized = false;

void edgewind_ui_on_before_enter_button(void)
{
}

void edgewind_ui_on_enter_system(void)
{
    if (!guider_initialized) {
        gui_assets_init();
        setup_ui(&guider_ui);
        gui_assets_patch_images(&guider_ui);
        guider_initialized = true;
        return;
    }

    if (guider_ui.Main_1) {
        lv_screen_load(guider_ui.Main_1);
    }
}
