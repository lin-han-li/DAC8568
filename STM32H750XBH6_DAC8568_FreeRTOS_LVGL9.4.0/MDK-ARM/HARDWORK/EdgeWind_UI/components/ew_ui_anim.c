/**
 * @file ew_ui_anim.c
 * @brief UI 动画工具实现
 */

#include "ew_ui_anim.h"

static void obj_opa_anim_cb(void *var, int32_t v)
{
    if (v < 0) v = 0;
    if (v > 255) v = 255;
    lv_obj_set_style_opa((lv_obj_t *)var, (lv_opa_t)v, 0);
}

void ew_ui_anim_fade_in(lv_obj_t *obj, uint32_t delay_ms, uint32_t dur_ms)
{
    if (!obj) return;

    lv_obj_set_style_opa(obj, LV_OPA_0, 0);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, 0, 255);
    lv_anim_set_duration(&a, dur_ms);
    lv_anim_set_delay(&a, delay_ms);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_set_exec_cb(&a, obj_opa_anim_cb);
    lv_anim_start(&a);
}

