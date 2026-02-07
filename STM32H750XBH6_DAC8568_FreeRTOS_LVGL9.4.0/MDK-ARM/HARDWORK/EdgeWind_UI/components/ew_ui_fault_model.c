/**
 * @file ew_ui_fault_model.c
 * @brief 6 个故障卡片的数据模型实现
 */

#include "ew_ui_fault_model.h"

const ew_fault_config_t ew_fault_list[EW_FAULT_COUNT] = {
    { "交流窜入", "AC Ingress",        LV_SYMBOL_CHARGE,       LV_COLOR_MAKE(0xFF, 0xC1, 0x07) }, /* AMBER */
    { "母线接地", "Bus Grounding",     LV_SYMBOL_SETTINGS,     LV_COLOR_MAKE(0x79, 0x55, 0x48) }, /* BROWN */
    { "绝缘劣化", "Insulation",       LV_SYMBOL_REFRESH,      LV_COLOR_MAKE(0x00, 0xBC, 0xD4) }, /* CYAN */
    { "电容老化", "Capacitor",        LV_SYMBOL_BATTERY_3,    LV_COLOR_MAKE(0xFF, 0x57, 0x22) }, /* DEEP_ORANGE */
    { "PWM异常",  "PWM Abnormality",  LV_SYMBOL_AUDIO,        LV_COLOR_MAKE(0x9C, 0x27, 0xB0) }, /* PURPLE */
    { "IGBT故障", "IGBT Failure",     LV_SYMBOL_CLOSE,        LV_COLOR_MAKE(0xF4, 0x43, 0x36) }, /* RED */
};

const ew_fault_config_t * ew_fault_get(uint32_t index)
{
    if (index >= EW_FAULT_COUNT) return NULL;
    return &ew_fault_list[index];
}
