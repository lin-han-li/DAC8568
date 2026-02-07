/**
 * @file ew_ui_anim.h
 * @brief UI 动画工具（避免直接强转 LVGL API）
 */

#ifndef EW_UI_ANIM_H
#define EW_UI_ANIM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include <stdint.h>

void ew_ui_anim_fade_in(lv_obj_t *obj, uint32_t delay_ms, uint32_t dur_ms);

#ifdef __cplusplus
}
#endif

#endif /* EW_UI_ANIM_H */

