/*
 * Copyright 2026 NXP
 * Replica of GUI Guider Generated Code for Gemini Project
 */

#include "../../gui_assets.h"
#include "custom.h"
#include "events_init.h"
#include "gui_guider.h"
#include "lvgl.h"
#include "widgets_init.h"
#include <stdio.h>

void setup_scr_WifiConfig(lv_ui *ui) {
  // Write codes WifiConfig (Screen)
  ui->WifiConfig = lv_obj_create(NULL);
  lv_obj_set_size(ui->WifiConfig, 320, 240);
  lv_obj_set_scrollbar_mode(ui->WifiConfig, LV_SCROLLBAR_MODE_OFF);

  // Write style for WifiConfig
  lv_obj_set_style_bg_opa(ui->WifiConfig, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(ui->WifiConfig, lv_color_hex(0x65adff),
                            LV_PART_MAIN | LV_STATE_DEFAULT);

  // Write codes header (match Main_x header)
  lv_obj_t *header = lv_obj_create(ui->WifiConfig);
  lv_obj_set_size(header, 320, 30);
  lv_obj_set_pos(header, 0, 0);
  lv_obj_set_style_bg_opa(header, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(header, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(header, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_scrollbar_mode(header, LV_SCROLLBAR_MODE_OFF);
  lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

  // Write codes WifiConfig_lbl_title (Header)
  ui->WifiConfig_lbl_title = lv_label_create(header);
  lv_obj_set_width(ui->WifiConfig_lbl_title, 300);
  lv_label_set_long_mode(ui->WifiConfig_lbl_title, LV_LABEL_LONG_CLIP);
  lv_label_set_text(ui->WifiConfig_lbl_title, "网络配置(WiFi)");
  lv_obj_set_style_text_color(ui->WifiConfig_lbl_title, lv_color_hex(0xFFFFFF),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui->WifiConfig_lbl_title, gui_assets_get_font_16(),
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_align(ui->WifiConfig_lbl_title, LV_ALIGN_LEFT_MID, 8, 0);

  // Write codes WifiConfig_cont_panel (White Box)
  ui->WifiConfig_cont_panel = lv_obj_create(ui->WifiConfig);
  lv_obj_set_pos(ui->WifiConfig_cont_panel, 10, 35);
  lv_obj_set_size(ui->WifiConfig_cont_panel, 300, 110);
  lv_obj_set_scrollbar_mode(ui->WifiConfig_cont_panel, LV_SCROLLBAR_MODE_OFF);
  lv_obj_set_style_bg_color(ui->WifiConfig_cont_panel, lv_color_hex(0xFFFFFF),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(ui->WifiConfig_cont_panel, 230,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(ui->WifiConfig_cont_panel, 8,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(ui->WifiConfig_cont_panel, 0,
                                LV_PART_MAIN | LV_STATE_DEFAULT);

  // Write codes WifiConfig_lbl_ssid
  ui->WifiConfig_lbl_ssid = lv_label_create(ui->WifiConfig_cont_panel);
  lv_obj_set_pos(ui->WifiConfig_lbl_ssid, 8, 5);
  lv_label_set_text(ui->WifiConfig_lbl_ssid, "WiFi SSID");
  lv_obj_set_style_text_color(ui->WifiConfig_lbl_ssid, lv_color_hex(0x2F35DA),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui->WifiConfig_lbl_ssid, gui_assets_get_font_12(),
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  // Write codes WifiConfig_ta_ssid
  ui->WifiConfig_ta_ssid = lv_textarea_create(ui->WifiConfig_cont_panel);
  lv_obj_set_pos(ui->WifiConfig_ta_ssid, 8, 22);
  lv_obj_set_size(ui->WifiConfig_ta_ssid, 270, 30);
  lv_textarea_set_one_line(ui->WifiConfig_ta_ssid, true);
  lv_textarea_set_placeholder_text(ui->WifiConfig_ta_ssid, "Enter SSID...");
  lv_obj_set_style_text_font(ui->WifiConfig_ta_ssid, gui_assets_get_font_12(),
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(ui->WifiConfig_ta_ssid, 1,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(ui->WifiConfig_ta_ssid, lv_color_hex(0xCCCCCC),
                                LV_PART_MAIN | LV_STATE_DEFAULT);

  // Write codes WifiConfig_lbl_pwd
  ui->WifiConfig_lbl_pwd = lv_label_create(ui->WifiConfig_cont_panel);
  lv_obj_set_pos(ui->WifiConfig_lbl_pwd, 8, 58);
  lv_label_set_text(ui->WifiConfig_lbl_pwd, "Password");
  lv_obj_set_style_text_color(ui->WifiConfig_lbl_pwd, lv_color_hex(0x2F35DA),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui->WifiConfig_lbl_pwd, gui_assets_get_font_12(),
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  // Write codes WifiConfig_ta_pwd
  ui->WifiConfig_ta_pwd = lv_textarea_create(ui->WifiConfig_cont_panel);
  lv_obj_set_pos(ui->WifiConfig_ta_pwd, 8, 75);
  lv_obj_set_size(ui->WifiConfig_ta_pwd, 270, 30);
  lv_textarea_set_one_line(ui->WifiConfig_ta_pwd, true);
  lv_textarea_set_password_mode(ui->WifiConfig_ta_pwd, true);
  lv_textarea_set_placeholder_text(ui->WifiConfig_ta_pwd, "Enter Password...");
  lv_obj_set_style_text_font(ui->WifiConfig_ta_pwd, gui_assets_get_font_12(),
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(ui->WifiConfig_ta_pwd, 1,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(ui->WifiConfig_ta_pwd, lv_color_hex(0xCCCCCC),
                                LV_PART_MAIN | LV_STATE_DEFAULT);

  // Write codes WifiConfig_lbl_status (Status Hint)
  ui->WifiConfig_lbl_status = lv_label_create(ui->WifiConfig);
  lv_obj_set_pos(ui->WifiConfig_lbl_status, 10, 150);
  lv_obj_set_size(ui->WifiConfig_lbl_status, 300, 20);
  lv_label_set_long_mode(ui->WifiConfig_lbl_status, LV_LABEL_LONG_DOT);
  lv_label_set_text(ui->WifiConfig_lbl_status, "状态：等待操作");
  lv_obj_set_style_text_color(ui->WifiConfig_lbl_status, lv_color_hex(0xFFFFFF),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui->WifiConfig_lbl_status,
                             gui_assets_get_font_12(),
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  // Write codes WifiConfig_btn_back (Gray)
  ui->WifiConfig_btn_back = lv_button_create(ui->WifiConfig);
  lv_obj_set_pos(ui->WifiConfig_btn_back, 10, 175);
  lv_obj_set_size(ui->WifiConfig_btn_back, 80, 30);
  lv_obj_set_style_bg_color(ui->WifiConfig_btn_back, lv_color_hex(0x999999),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(ui->WifiConfig_btn_back, 15,
                          LV_PART_MAIN | LV_STATE_DEFAULT);

  ui->WifiConfig_btn_back_label = lv_label_create(ui->WifiConfig_btn_back);
  lv_label_set_text(ui->WifiConfig_btn_back_label, "返回");
  lv_obj_set_style_text_font(ui->WifiConfig_btn_back_label,
                             gui_assets_get_font_12(),
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_center(ui->WifiConfig_btn_back_label);

  // Write codes WifiConfig_btn_scan (Orange)
  ui->WifiConfig_btn_scan = lv_button_create(ui->WifiConfig);
  lv_obj_set_pos(ui->WifiConfig_btn_scan, 120, 175);
  lv_obj_set_size(ui->WifiConfig_btn_scan, 80, 30);
  lv_obj_set_style_bg_color(ui->WifiConfig_btn_scan, lv_color_hex(0xFFA500),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(ui->WifiConfig_btn_scan, 15,
                          LV_PART_MAIN | LV_STATE_DEFAULT);

  ui->WifiConfig_btn_scan_label = lv_label_create(ui->WifiConfig_btn_scan);
  lv_label_set_text(ui->WifiConfig_btn_scan_label, "读取配置");
  lv_obj_set_style_text_font(ui->WifiConfig_btn_scan_label,
                             gui_assets_get_font_12(),
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_center(ui->WifiConfig_btn_scan_label);

  // Write codes WifiConfig_btn_save (Green)
  ui->WifiConfig_btn_save = lv_button_create(ui->WifiConfig);
  lv_obj_set_pos(ui->WifiConfig_btn_save, 230, 175);
  lv_obj_set_size(ui->WifiConfig_btn_save, 80, 30);
  lv_obj_set_style_bg_color(ui->WifiConfig_btn_save, lv_color_hex(0x3dfb00),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(ui->WifiConfig_btn_save, 15,
                          LV_PART_MAIN | LV_STATE_DEFAULT);

  ui->WifiConfig_btn_save_label = lv_label_create(ui->WifiConfig_btn_save);
  lv_label_set_text(ui->WifiConfig_btn_save_label, "保存配置");
  lv_obj_set_style_text_color(ui->WifiConfig_btn_save_label,
                              lv_color_hex(0x2F35DA),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui->WifiConfig_btn_save_label,
                             gui_assets_get_font_12(),
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_center(ui->WifiConfig_btn_save_label);

  // Write codes WifiConfig_kb (Keyboard)
  ui->WifiConfig_kb = lv_keyboard_create(ui->WifiConfig);
  lv_obj_set_size(ui->WifiConfig_kb, 320, 120);
  lv_obj_align(ui->WifiConfig_kb, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_add_flag(ui->WifiConfig_kb, LV_OBJ_FLAG_HIDDEN); // Initially hidden

  // Update current screen layout.
  lv_obj_update_layout(ui->WifiConfig);

  // Init events
  events_init_WifiConfig(ui);
}
