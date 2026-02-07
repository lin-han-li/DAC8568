/**
 * @file ew_fonts.h
 * @brief EdgeWind UI 字库声明
 * 
 * 使用 GUI Guider 生成的思源宋体 (Source Han Serif SC) 字库
 * 包含项目所需的完整中文字符集
 */

#ifndef EW_FONTS_H
#define EW_FONTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

/*******************************************************************************
 * 思源宋体字库声明（项目使用 12/14/16px）
 ******************************************************************************/

LV_FONT_DECLARE(lv_font_SourceHanSerifSC_Regular_12);
LV_FONT_DECLARE(lv_font_SourceHanSerifSC_Regular_14);
LV_FONT_DECLARE(lv_font_SourceHanSerifSC_Regular_16);

/*******************************************************************************
 * 字体别名（方便使用）
 * 
 * 中文使用 GUI Guider 生成的思源宋体，英文数字使用 Montserrat
 ******************************************************************************/

/* 中文字体 */
#define EW_FONT_CN_12       (&lv_font_SourceHanSerifSC_Regular_12)  /* 12px 次级/徽章/页脚 */
#define EW_FONT_CN_14       (&lv_font_SourceHanSerifSC_Regular_14)  /* 14px 通用/开机按钮 */
#define EW_FONT_CN_16       (&lv_font_SourceHanSerifSC_Regular_16)  /* 16px 主界面标题 */

#define EW_FONT_CN_SMALL    EW_FONT_CN_12
#define EW_FONT_CN_NORMAL   EW_FONT_CN_14
#define EW_FONT_CN_TITLE    EW_FONT_CN_16
#define EW_FONT_CN_LARGE    EW_FONT_CN_16
#define EW_FONT_CN_HOME_LABEL EW_FONT_CN_16

/* 英文/数字字体（使用 Montserrat） */
#define EW_FONT_EN_SMALL    (&lv_font_montserrat_14)                /* 14px 英文小字 */
#define EW_FONT_EN_NORMAL   (&lv_font_montserrat_16)                /* 16px 英文正常 */
#define EW_FONT_EN_TITLE    (&lv_font_montserrat_20)                /* 20px 英文标题 */
#define EW_FONT_EN_LARGE    (&lv_font_montserrat_28)                /* 28px 英文大数值 */

#ifdef __cplusplus
}
#endif

#endif /* EW_FONTS_H */
