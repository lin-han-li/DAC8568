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

void setup_scr_ServerConfig(lv_ui *ui) {
  // Write codes ServerConfig (Screen)
  ui->ServerConfig = lv_obj_create(NULL);
  lv_obj_set_size(ui->ServerConfig, 320, 240);
  lv_obj_set_scrollbar_mode(ui->ServerConfig, LV_SCROLLBAR_MODE_OFF);

  // Write style for ServerConfig
  lv_obj_set_style_bg_opa(ui->ServerConfig, 255,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(ui->ServerConfig, lv_color_hex(0x65adff),
                            LV_PART_MAIN | LV_STATE_DEFAULT);

  // Header (match Main_x header)
  lv_obj_t *header = lv_obj_create(ui->ServerConfig);
  lv_obj_set_size(header, 320, 30);
  lv_obj_set_pos(header, 0, 0);
  lv_obj_set_style_bg_opa(header, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(header, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(header, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_scrollbar_mode(header, LV_SCROLLBAR_MODE_OFF);
  lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

  ui->ServerConfig_lbl_title = lv_label_create(header);
  lv_obj_set_width(ui->ServerConfig_lbl_title, 300);
  lv_label_set_long_mode(ui->ServerConfig_lbl_title, LV_LABEL_LONG_CLIP);
  lv_label_set_text(ui->ServerConfig_lbl_title, "服务器配置(Server)");
  lv_obj_set_style_text_color(ui->ServerConfig_lbl_title,
                              lv_color_hex(0xFFFFFF),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui->ServerConfig_lbl_title,
                             gui_assets_get_font_16(),
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_align(ui->ServerConfig_lbl_title, LV_ALIGN_LEFT_MID, 8, 0);

  // Container
  ui->ServerConfig_cont_panel = lv_obj_create(ui->ServerConfig);
  lv_obj_set_pos(ui->ServerConfig_cont_panel, 10, 35);
  lv_obj_set_size(ui->ServerConfig_cont_panel, 300, 130);
  lv_obj_set_style_bg_color(ui->ServerConfig_cont_panel, lv_color_hex(0xFFFFFF),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(ui->ServerConfig_cont_panel, 230,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(ui->ServerConfig_cont_panel, 8,
                          LV_PART_MAIN | LV_STATE_DEFAULT);

  // --- Row 1: IP & Port ---

  // IP Label
  ui->ServerConfig_lbl_ip = lv_label_create(ui->ServerConfig_cont_panel);
  lv_obj_set_pos(ui->ServerConfig_lbl_ip, 8, 5);
  lv_label_set_text(ui->ServerConfig_lbl_ip, "Server IP");
  lv_obj_set_style_text_color(ui->ServerConfig_lbl_ip, lv_color_hex(0x2F35DA),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui->ServerConfig_lbl_ip, gui_assets_get_font_12(),
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  // IP TextArea
  ui->ServerConfig_ta_ip = lv_textarea_create(ui->ServerConfig_cont_panel);
  lv_obj_set_pos(ui->ServerConfig_ta_ip, 8, 20);
  lv_obj_set_size(ui->ServerConfig_ta_ip, 170, 28);
  lv_textarea_set_one_line(ui->ServerConfig_ta_ip, true);
  lv_textarea_set_text(ui->ServerConfig_ta_ip, "192.168.10.43");
  lv_obj_set_style_text_font(ui->ServerConfig_ta_ip, gui_assets_get_font_12(),
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(ui->ServerConfig_ta_ip, 1,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(ui->ServerConfig_ta_ip, lv_color_hex(0xCCCCCC),
                                LV_PART_MAIN | LV_STATE_DEFAULT);

  // Port Label
  ui->ServerConfig_lbl_port = lv_label_create(ui->ServerConfig_cont_panel);
  lv_obj_set_pos(ui->ServerConfig_lbl_port, 190, 5);
  lv_label_set_text(ui->ServerConfig_lbl_port, "Port");
  lv_obj_set_style_text_color(ui->ServerConfig_lbl_port, lv_color_hex(0x2F35DA),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui->ServerConfig_lbl_port,
                             gui_assets_get_font_12(),
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  // Port TextArea
  ui->ServerConfig_ta_port = lv_textarea_create(ui->ServerConfig_cont_panel);
  lv_obj_set_pos(ui->ServerConfig_ta_port, 190, 20);
  lv_obj_set_size(ui->ServerConfig_ta_port, 96, 28);
  lv_textarea_set_one_line(ui->ServerConfig_ta_port, true);
  lv_textarea_set_text(ui->ServerConfig_ta_port, "5000");
  lv_obj_set_style_text_font(ui->ServerConfig_ta_port, gui_assets_get_font_12(),
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(ui->ServerConfig_ta_port, 1,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(ui->ServerConfig_ta_port,
                                lv_color_hex(0xCCCCCC),
                                LV_PART_MAIN | LV_STATE_DEFAULT);

  // --- Row 2: Node ID & Location ---

  // ID Label
  ui->ServerConfig_lbl_id = lv_label_create(ui->ServerConfig_cont_panel);
  lv_obj_set_pos(ui->ServerConfig_lbl_id, 8, 55);
  lv_obj_set_size(ui->ServerConfig_lbl_id, 170, LV_SIZE_CONTENT);
  lv_label_set_long_mode(ui->ServerConfig_lbl_id, LV_LABEL_LONG_CLIP);
  lv_label_set_text(ui->ServerConfig_lbl_id, "Device ID");
  lv_obj_set_style_text_color(ui->ServerConfig_lbl_id, lv_color_hex(0x2F35DA),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui->ServerConfig_lbl_id, gui_assets_get_font_12(),
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  // ID TextArea
  ui->ServerConfig_ta_id = lv_textarea_create(ui->ServerConfig_cont_panel);
  lv_obj_set_pos(ui->ServerConfig_ta_id, 8, 70);
  lv_obj_set_size(ui->ServerConfig_ta_id, 130, 28);
  lv_textarea_set_one_line(ui->ServerConfig_ta_id, true);
  lv_textarea_set_text(ui->ServerConfig_ta_id, "STM32_H7_Node");
  lv_obj_set_style_text_font(ui->ServerConfig_ta_id, gui_assets_get_font_12(),
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(ui->ServerConfig_ta_id, 1,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(ui->ServerConfig_ta_id, lv_color_hex(0xCCCCCC),
                                LV_PART_MAIN | LV_STATE_DEFAULT);

  // Location Label
  ui->ServerConfig_lbl_loc = lv_label_create(ui->ServerConfig_cont_panel);
  lv_obj_set_pos(ui->ServerConfig_lbl_loc, 150, 55);
  lv_obj_set_size(ui->ServerConfig_lbl_loc, 136, LV_SIZE_CONTENT);
  lv_label_set_long_mode(ui->ServerConfig_lbl_loc, LV_LABEL_LONG_CLIP);
  lv_label_set_text(ui->ServerConfig_lbl_loc, "Location");
  lv_obj_set_style_text_color(ui->ServerConfig_lbl_loc, lv_color_hex(0x2F35DA),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui->ServerConfig_lbl_loc, gui_assets_get_font_12(),
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  // Location TextArea
  ui->ServerConfig_ta_loc = lv_textarea_create(ui->ServerConfig_cont_panel);
  lv_obj_set_pos(ui->ServerConfig_ta_loc, 150, 70);
  lv_obj_set_size(ui->ServerConfig_ta_loc, 136, 28);
  lv_textarea_set_one_line(ui->ServerConfig_ta_loc, true);
  lv_textarea_set_placeholder_text(ui->ServerConfig_ta_loc, "Location...");
  lv_obj_set_style_text_font(ui->ServerConfig_ta_loc, gui_assets_get_font_12(),
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(ui->ServerConfig_ta_loc, 1,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(ui->ServerConfig_ta_loc, lv_color_hex(0xCCCCCC),
                                LV_PART_MAIN | LV_STATE_DEFAULT);

  // --- Buttons ---

  // Back Button
  ui->ServerConfig_btn_back = lv_button_create(ui->ServerConfig);
  lv_obj_set_pos(ui->ServerConfig_btn_back, 10, 195);
  lv_obj_set_size(ui->ServerConfig_btn_back, 80, 30);
  lv_obj_set_style_bg_color(ui->ServerConfig_btn_back, lv_color_hex(0x999999),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(ui->ServerConfig_btn_back, 15,
                          LV_PART_MAIN | LV_STATE_DEFAULT);

  ui->ServerConfig_btn_back_label = lv_label_create(ui->ServerConfig_btn_back);
  lv_label_set_text(ui->ServerConfig_btn_back_label, "返回");
  lv_obj_set_style_text_font(ui->ServerConfig_btn_back_label,
                             gui_assets_get_font_12(),
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_center(ui->ServerConfig_btn_back_label);

  // Load Button (Orange)
  ui->ServerConfig_btn_load = lv_button_create(ui->ServerConfig);
  lv_obj_set_pos(ui->ServerConfig_btn_load, 120, 195);
  lv_obj_set_size(ui->ServerConfig_btn_load, 80, 30);
  lv_obj_set_style_bg_color(ui->ServerConfig_btn_load, lv_color_hex(0xFFA500),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(ui->ServerConfig_btn_load, 15,
                          LV_PART_MAIN | LV_STATE_DEFAULT);

  ui->ServerConfig_btn_load_label = lv_label_create(ui->ServerConfig_btn_load);
  lv_label_set_text(ui->ServerConfig_btn_load_label, "读取配置");
  lv_obj_set_style_text_color(ui->ServerConfig_btn_load_label,
                              lv_color_hex(0xFFFFFF),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui->ServerConfig_btn_load_label,
                             gui_assets_get_font_12(),
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_center(ui->ServerConfig_btn_load_label);

  // Save Button (Green)
  ui->ServerConfig_btn_save = lv_button_create(ui->ServerConfig);
  lv_obj_set_pos(ui->ServerConfig_btn_save, 230, 195);
  lv_obj_set_size(ui->ServerConfig_btn_save, 80, 30);
  lv_obj_set_style_bg_color(ui->ServerConfig_btn_save, lv_color_hex(0x3dfb00),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(ui->ServerConfig_btn_save, 15,
                          LV_PART_MAIN | LV_STATE_DEFAULT);

  ui->ServerConfig_btn_save_label = lv_label_create(ui->ServerConfig_btn_save);
  lv_label_set_text(ui->ServerConfig_btn_save_label, "保存配置");
  lv_obj_set_style_text_color(ui->ServerConfig_btn_save_label,
                              lv_color_hex(0x2F35DA),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui->ServerConfig_btn_save_label,
                             gui_assets_get_font_12(),
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_center(ui->ServerConfig_btn_save_label);

  // Status Label
  ui->ServerConfig_lbl_status = lv_label_create(ui->ServerConfig);
  lv_obj_set_pos(ui->ServerConfig_lbl_status, 10, 175);
  lv_label_set_text(ui->ServerConfig_lbl_status, "状态：等待操作");
  lv_obj_set_style_text_color(ui->ServerConfig_lbl_status,
                              lv_color_hex(0xFFFFFF),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui->ServerConfig_lbl_status,
                             gui_assets_get_font_12(),
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_width(ui->ServerConfig_lbl_status, 300);

  // Keyboard
  ui->ServerConfig_kb = lv_keyboard_create(ui->ServerConfig);
  lv_obj_set_size(ui->ServerConfig_kb, 320, 120);
  lv_obj_align(ui->ServerConfig_kb, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_add_flag(ui->ServerConfig_kb, LV_OBJ_FLAG_HIDDEN);

  // Update layout
  lv_obj_update_layout(ui->ServerConfig);
  events_init_ServerConfig(ui);
}
