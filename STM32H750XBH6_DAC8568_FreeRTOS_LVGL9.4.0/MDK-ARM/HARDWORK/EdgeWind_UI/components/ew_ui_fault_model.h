/**
 * @file ew_ui_fault_model.h
 * @brief 6 个故障卡片的数据模型
 */

#ifndef EW_UI_FAULT_MODEL_H
#define EW_UI_FAULT_MODEL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include <stdint.h>

#define EW_FAULT_COUNT 6u

typedef struct {
    const char * name_cn;
    const char * name_en;
    const char * icon;
    lv_color_t theme_color;
} ew_fault_config_t;

extern const ew_fault_config_t ew_fault_list[EW_FAULT_COUNT];

const ew_fault_config_t * ew_fault_get(uint32_t index);

#ifdef __cplusplus
}
#endif

#endif /* EW_UI_FAULT_MODEL_H */

