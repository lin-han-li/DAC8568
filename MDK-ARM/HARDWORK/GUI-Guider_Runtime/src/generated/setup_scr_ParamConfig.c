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

void setup_scr_ParamConfig(lv_ui *ui) {
  // Write codes ParamConfig (Screen)
  ui->ParamConfig = lv_obj_create(NULL);
  lv_obj_set_size(ui->ParamConfig, 320, 240);
  lv_obj_set_scrollbar_mode(ui->ParamConfig, LV_SCROLLBAR_MODE_OFF);

  // Write style for ParamConfig
  lv_obj_set_style_bg_opa(ui->ParamConfig, 255,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(ui->ParamConfig, lv_color_hex(0x65adff),
                            LV_PART_MAIN | LV_STATE_DEFAULT);

  // Header (match Main_x header)
  lv_obj_t *header = lv_obj_create(ui->ParamConfig);
  lv_obj_set_size(header, 320, 30);
  lv_obj_set_pos(header, 0, 0);
  lv_obj_set_style_bg_opa(header, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(header, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(header, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_scrollbar_mode(header, LV_SCROLLBAR_MODE_OFF);
  lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

  // Write codes ParamConfig_lbl_title (Header)
  ui->ParamConfig_lbl_title = lv_label_create(header);
  lv_obj_set_width(ui->ParamConfig_lbl_title, 180);
  lv_label_set_long_mode(ui->ParamConfig_lbl_title, LV_LABEL_LONG_CLIP);
  lv_label_set_text(ui->ParamConfig_lbl_title, "通讯参数配置");
  lv_obj_set_style_text_color(ui->ParamConfig_lbl_title, lv_color_hex(0xFFFFFF),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui->ParamConfig_lbl_title,
                             gui_assets_get_font_16(),
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_align(ui->ParamConfig_lbl_title, LV_ALIGN_LEFT_MID, 8, 0);

  /* Header quick presets: LAN / WAN */
  ui->ParamConfig_btn_wan = lv_button_create(header);
  lv_obj_set_size(ui->ParamConfig_btn_wan, 50, 22);
  lv_obj_align(ui->ParamConfig_btn_wan, LV_ALIGN_RIGHT_MID, -8, 0);
  lv_obj_set_style_radius(ui->ParamConfig_btn_wan, 11,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(ui->ParamConfig_btn_wan, lv_color_hex(0xFFA500),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(ui->ParamConfig_btn_wan, 255,
                          LV_PART_MAIN | LV_STATE_DEFAULT);

  ui->ParamConfig_lbl_wan = lv_label_create(ui->ParamConfig_btn_wan);
  lv_label_set_text(ui->ParamConfig_lbl_wan, "公网");
  lv_obj_set_style_text_font(ui->ParamConfig_lbl_wan, gui_assets_get_font_12(),
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(ui->ParamConfig_lbl_wan, lv_color_hex(0x000000),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_center(ui->ParamConfig_lbl_wan);

  ui->ParamConfig_btn_lan = lv_button_create(header);
  lv_obj_set_size(ui->ParamConfig_btn_lan, 50, 22);
  lv_obj_align_to(ui->ParamConfig_btn_lan, ui->ParamConfig_btn_wan,
                  LV_ALIGN_OUT_LEFT_MID, -4, 0);
  lv_obj_set_style_radius(ui->ParamConfig_btn_lan, 11,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(ui->ParamConfig_btn_lan, lv_color_hex(0x3dfb00),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(ui->ParamConfig_btn_lan, 255,
                          LV_PART_MAIN | LV_STATE_DEFAULT);

  ui->ParamConfig_lbl_lan = lv_label_create(ui->ParamConfig_btn_lan);
  lv_label_set_text(ui->ParamConfig_lbl_lan, "局域网");
  lv_obj_set_style_text_font(ui->ParamConfig_lbl_lan, gui_assets_get_font_12(),
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(ui->ParamConfig_lbl_lan, lv_color_hex(0x000000),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_center(ui->ParamConfig_lbl_lan);

  // Container — scrollable to fit all params on small screen
  ui->ParamConfig_cont_panel = lv_obj_create(ui->ParamConfig);
  lv_obj_set_pos(ui->ParamConfig_cont_panel, 10, 35);
  lv_obj_set_size(ui->ParamConfig_cont_panel, 300, 130);
  lv_obj_set_scrollbar_mode(ui->ParamConfig_cont_panel, LV_SCROLLBAR_MODE_AUTO);
  lv_obj_set_style_bg_color(ui->ParamConfig_cont_panel, lv_color_hex(0xFFFFFF),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(ui->ParamConfig_cont_panel, 230,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(ui->ParamConfig_cont_panel, 8,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(ui->ParamConfig_cont_panel, 0,
                                LV_PART_MAIN | LV_STATE_DEFAULT);

  // --- Row 1: Heartbeat & Send Limit ---
  ui->ParamConfig_lbl_heartbeat = lv_label_create(ui->ParamConfig_cont_panel);
  lv_obj_set_pos(ui->ParamConfig_lbl_heartbeat, 8, 0);
  lv_label_set_text(ui->ParamConfig_lbl_heartbeat, "心跳(ms)");
  lv_obj_set_style_text_color(ui->ParamConfig_lbl_heartbeat,
                              lv_color_hex(0x2F35DA),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui->ParamConfig_lbl_heartbeat,
                             gui_assets_get_font_12(),
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  ui->ParamConfig_ta_heartbeat = lv_textarea_create(ui->ParamConfig_cont_panel);
  lv_obj_set_pos(ui->ParamConfig_ta_heartbeat, 8, 16);
  lv_obj_set_size(ui->ParamConfig_ta_heartbeat, 130, 26);
  lv_textarea_set_one_line(ui->ParamConfig_ta_heartbeat, true);
  lv_textarea_set_accepted_chars(ui->ParamConfig_ta_heartbeat, "0123456789");
  lv_textarea_set_text(ui->ParamConfig_ta_heartbeat, "5000");
  lv_obj_set_style_text_font(ui->ParamConfig_ta_heartbeat,
                             gui_assets_get_font_12(),
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(ui->ParamConfig_ta_heartbeat, 1,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(ui->ParamConfig_ta_heartbeat,
                                lv_color_hex(0xCCCCCC),
                                LV_PART_MAIN | LV_STATE_DEFAULT);

  ui->ParamConfig_lbl_sendlimit = lv_label_create(ui->ParamConfig_cont_panel);
  lv_obj_set_pos(ui->ParamConfig_lbl_sendlimit, 150, 0);
  lv_label_set_text(ui->ParamConfig_lbl_sendlimit, "限频(ms)");
  lv_obj_set_style_text_color(ui->ParamConfig_lbl_sendlimit,
                              lv_color_hex(0x2F35DA),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui->ParamConfig_lbl_sendlimit,
                             gui_assets_get_font_12(),
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  ui->ParamConfig_ta_sendlimit = lv_textarea_create(ui->ParamConfig_cont_panel);
  lv_obj_set_pos(ui->ParamConfig_ta_sendlimit, 150, 16);
  lv_obj_set_size(ui->ParamConfig_ta_sendlimit, 130, 26);
  lv_textarea_set_one_line(ui->ParamConfig_ta_sendlimit, true);
  lv_textarea_set_accepted_chars(ui->ParamConfig_ta_sendlimit, "0123456789");
  lv_textarea_set_text(ui->ParamConfig_ta_sendlimit, "200");
  lv_obj_set_style_text_font(ui->ParamConfig_ta_sendlimit,
                             gui_assets_get_font_12(),
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(ui->ParamConfig_ta_sendlimit, 1,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(ui->ParamConfig_ta_sendlimit,
                                lv_color_hex(0xCCCCCC),
                                LV_PART_MAIN | LV_STATE_DEFAULT);

  // --- Row 2: HTTP Timeout & Hard Reset ---
  ui->ParamConfig_lbl_httptimeout = lv_label_create(ui->ParamConfig_cont_panel);
  lv_obj_set_pos(ui->ParamConfig_lbl_httptimeout, 8, 48);
  lv_label_set_text(ui->ParamConfig_lbl_httptimeout, "回包超时(ms)");
  lv_obj_set_style_text_color(ui->ParamConfig_lbl_httptimeout,
                              lv_color_hex(0x2F35DA),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui->ParamConfig_lbl_httptimeout,
                             gui_assets_get_font_12(),
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  ui->ParamConfig_ta_httptimeout =
      lv_textarea_create(ui->ParamConfig_cont_panel);
  lv_obj_set_pos(ui->ParamConfig_ta_httptimeout, 8, 64);
  lv_obj_set_size(ui->ParamConfig_ta_httptimeout, 130, 26);
  lv_textarea_set_one_line(ui->ParamConfig_ta_httptimeout, true);
  lv_textarea_set_accepted_chars(ui->ParamConfig_ta_httptimeout, "0123456789");
  lv_textarea_set_text(ui->ParamConfig_ta_httptimeout, "1200");
  lv_obj_set_style_text_font(ui->ParamConfig_ta_httptimeout,
                             gui_assets_get_font_12(),
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(ui->ParamConfig_ta_httptimeout, 1,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(ui->ParamConfig_ta_httptimeout,
                                lv_color_hex(0xCCCCCC),
                                LV_PART_MAIN | LV_STATE_DEFAULT);

  ui->ParamConfig_lbl_hardreset = lv_label_create(ui->ParamConfig_cont_panel);
  lv_obj_set_pos(ui->ParamConfig_lbl_hardreset, 150, 48);
  lv_label_set_text(ui->ParamConfig_lbl_hardreset, "复位(s)");
  lv_obj_set_style_text_color(ui->ParamConfig_lbl_hardreset,
                              lv_color_hex(0x2F35DA),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui->ParamConfig_lbl_hardreset,
                             gui_assets_get_font_12(),
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  ui->ParamConfig_ta_hardreset = lv_textarea_create(ui->ParamConfig_cont_panel);
  lv_obj_set_pos(ui->ParamConfig_ta_hardreset, 150, 64);
  lv_obj_set_size(ui->ParamConfig_ta_hardreset, 130, 26);
  lv_textarea_set_one_line(ui->ParamConfig_ta_hardreset, true);
  lv_textarea_set_accepted_chars(ui->ParamConfig_ta_hardreset, "0123456789");
  lv_textarea_set_text(ui->ParamConfig_ta_hardreset, "60");
  lv_obj_set_style_text_font(ui->ParamConfig_ta_hardreset,
                             gui_assets_get_font_12(),
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(ui->ParamConfig_ta_hardreset, 1,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(ui->ParamConfig_ta_hardreset,
                                lv_color_hex(0xCCCCCC),
                                LV_PART_MAIN | LV_STATE_DEFAULT);

  // --- Row 3: Chunk & Delay ---
  ui->ParamConfig_lbl_chunkkb = lv_label_create(ui->ParamConfig_cont_panel);
  lv_obj_set_pos(ui->ParamConfig_lbl_chunkkb, 8, 96);
  lv_label_set_text(ui->ParamConfig_lbl_chunkkb, "段(KB)");
  lv_obj_set_style_text_color(ui->ParamConfig_lbl_chunkkb,
                              lv_color_hex(0x2F35DA),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui->ParamConfig_lbl_chunkkb,
                             gui_assets_get_font_12(),
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  ui->ParamConfig_ta_chunkkb = lv_textarea_create(ui->ParamConfig_cont_panel);
  lv_obj_set_pos(ui->ParamConfig_ta_chunkkb, 8, 112);
  lv_obj_set_size(ui->ParamConfig_ta_chunkkb, 60, 26);
  lv_textarea_set_one_line(ui->ParamConfig_ta_chunkkb, true);
  lv_textarea_set_accepted_chars(ui->ParamConfig_ta_chunkkb, "0123456789");
  lv_textarea_set_max_length(ui->ParamConfig_ta_chunkkb, 2);
  lv_textarea_set_text(ui->ParamConfig_ta_chunkkb, "4");
  lv_obj_set_style_text_font(ui->ParamConfig_ta_chunkkb,
                             gui_assets_get_font_12(),
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(ui->ParamConfig_ta_chunkkb, 1,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(ui->ParamConfig_ta_chunkkb,
                                lv_color_hex(0xCCCCCC),
                                LV_PART_MAIN | LV_STATE_DEFAULT);

  ui->ParamConfig_lbl_chunkdelay = lv_label_create(ui->ParamConfig_cont_panel);
  lv_obj_set_pos(ui->ParamConfig_lbl_chunkdelay, 80, 96);
  lv_label_set_text(ui->ParamConfig_lbl_chunkdelay, "间隔(ms)");
  lv_obj_set_style_text_color(ui->ParamConfig_lbl_chunkdelay,
                              lv_color_hex(0x2F35DA),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui->ParamConfig_lbl_chunkdelay,
                             gui_assets_get_font_12(),
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  ui->ParamConfig_ta_chunkdelay =
      lv_textarea_create(ui->ParamConfig_cont_panel);
  lv_obj_set_pos(ui->ParamConfig_ta_chunkdelay, 80, 112);
  lv_obj_set_size(ui->ParamConfig_ta_chunkdelay, 60, 26);
  lv_textarea_set_one_line(ui->ParamConfig_ta_chunkdelay, true);
  lv_textarea_set_accepted_chars(ui->ParamConfig_ta_chunkdelay, "0123456789");
  lv_textarea_set_max_length(ui->ParamConfig_ta_chunkdelay, 3);
  lv_textarea_set_text(ui->ParamConfig_ta_chunkdelay, "10");
  lv_obj_set_style_text_font(ui->ParamConfig_ta_chunkdelay,
                             gui_assets_get_font_12(),
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(ui->ParamConfig_ta_chunkdelay, 1,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(ui->ParamConfig_ta_chunkdelay,
                                lv_color_hex(0xCCCCCC),
                                LV_PART_MAIN | LV_STATE_DEFAULT);

  // No-chunk button
  ui->ParamConfig_btn_nochunk = lv_button_create(ui->ParamConfig_cont_panel);
  lv_obj_set_pos(ui->ParamConfig_btn_nochunk, 150, 108);
  lv_obj_set_size(ui->ParamConfig_btn_nochunk, 80, 20);
  lv_obj_set_style_radius(ui->ParamConfig_btn_nochunk, 10,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(ui->ParamConfig_btn_nochunk, lv_color_hex(0x6C757D),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(ui->ParamConfig_btn_nochunk, 200,
                          LV_PART_MAIN | LV_STATE_DEFAULT);

  ui->ParamConfig_lbl_nochunk = lv_label_create(ui->ParamConfig_btn_nochunk);
  lv_label_set_text(ui->ParamConfig_lbl_nochunk, "无分段");
  lv_obj_set_style_text_color(ui->ParamConfig_lbl_nochunk,
                              lv_color_hex(0xFFFFFF),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui->ParamConfig_lbl_nochunk,
                             gui_assets_get_font_12(),
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_center(ui->ParamConfig_lbl_nochunk);

  // --- Downsample ---
  ui->ParamConfig_lbl_downsample = lv_label_create(ui->ParamConfig_cont_panel);
  lv_obj_set_pos(ui->ParamConfig_lbl_downsample, 150, 96);
  lv_label_set_text(ui->ParamConfig_lbl_downsample, "降采样step");
  lv_obj_set_style_text_color(ui->ParamConfig_lbl_downsample,
                              lv_color_hex(0x2F35DA),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui->ParamConfig_lbl_downsample,
                             gui_assets_get_font_12(),
                             LV_PART_MAIN | LV_STATE_DEFAULT);

  ui->ParamConfig_ta_downsample =
      lv_textarea_create(ui->ParamConfig_cont_panel);
  lv_obj_set_pos(ui->ParamConfig_ta_downsample, 240, 96);
  lv_obj_set_size(ui->ParamConfig_ta_downsample, 40, 26);
  lv_textarea_set_one_line(ui->ParamConfig_ta_downsample, true);
  lv_textarea_set_accepted_chars(ui->ParamConfig_ta_downsample, "0123456789");
  lv_textarea_set_max_length(ui->ParamConfig_ta_downsample, 2);
  lv_textarea_set_text(ui->ParamConfig_ta_downsample, "1");
  lv_obj_set_style_text_font(ui->ParamConfig_ta_downsample,
                             gui_assets_get_font_12(),
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(ui->ParamConfig_ta_downsample, 1,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(ui->ParamConfig_ta_downsample,
                                lv_color_hex(0xCCCCCC),
                                LV_PART_MAIN | LV_STATE_DEFAULT);

  /* Tips */
  ui->ParamConfig_lbl_tips = lv_label_create(ui->ParamConfig_cont_panel);
  lv_obj_set_pos(ui->ParamConfig_lbl_tips, 8, 142);
  lv_obj_set_size(ui->ParamConfig_lbl_tips, 270, 50);
  lv_label_set_long_mode(ui->ParamConfig_lbl_tips, LV_LABEL_LONG_WRAP);
  lv_label_set_text(ui->ParamConfig_lbl_tips,
                    "建议：心跳5000，限频200，回包1200，复位60s\n"
                    "降采样：step=1全量，4推荐；分段4KB/10ms");
  lv_obj_set_style_text_font(ui->ParamConfig_lbl_tips, gui_assets_get_font_12(),
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(ui->ParamConfig_lbl_tips, lv_color_hex(0x111111),
                              LV_PART_MAIN | LV_STATE_DEFAULT);

  // --- Buttons ---
  ui->ParamConfig_btn_back = lv_button_create(ui->ParamConfig);
  lv_obj_set_pos(ui->ParamConfig_btn_back, 10, 195);
  lv_obj_set_size(ui->ParamConfig_btn_back, 80, 30);
  lv_obj_set_style_bg_color(ui->ParamConfig_btn_back, lv_color_hex(0x999999),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(ui->ParamConfig_btn_back, 15,
                          LV_PART_MAIN | LV_STATE_DEFAULT);

  ui->ParamConfig_btn_back_label = lv_label_create(ui->ParamConfig_btn_back);
  lv_label_set_text(ui->ParamConfig_btn_back_label, "返回");
  lv_obj_set_style_text_font(ui->ParamConfig_btn_back_label,
                             gui_assets_get_font_12(),
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_center(ui->ParamConfig_btn_back_label);

  // Load Button (Orange)
  ui->ParamConfig_btn_load = lv_button_create(ui->ParamConfig);
  lv_obj_set_pos(ui->ParamConfig_btn_load, 120, 195);
  lv_obj_set_size(ui->ParamConfig_btn_load, 80, 30);
  lv_obj_set_style_bg_color(ui->ParamConfig_btn_load, lv_color_hex(0xFFA500),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(ui->ParamConfig_btn_load, 15,
                          LV_PART_MAIN | LV_STATE_DEFAULT);

  ui->ParamConfig_btn_load_label = lv_label_create(ui->ParamConfig_btn_load);
  lv_label_set_text(ui->ParamConfig_btn_load_label, "读取配置");
  lv_obj_set_style_text_font(ui->ParamConfig_btn_load_label,
                             gui_assets_get_font_12(),
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_center(ui->ParamConfig_btn_load_label);

  // Save Button (Green)
  ui->ParamConfig_btn_save = lv_button_create(ui->ParamConfig);
  lv_obj_set_pos(ui->ParamConfig_btn_save, 230, 195);
  lv_obj_set_size(ui->ParamConfig_btn_save, 80, 30);
  lv_obj_set_style_bg_color(ui->ParamConfig_btn_save, lv_color_hex(0x3dfb00),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(ui->ParamConfig_btn_save, 15,
                          LV_PART_MAIN | LV_STATE_DEFAULT);

  ui->ParamConfig_btn_save_label = lv_label_create(ui->ParamConfig_btn_save);
  lv_label_set_text(ui->ParamConfig_btn_save_label, "保存配置");
  lv_obj_set_style_text_color(ui->ParamConfig_btn_save_label,
                              lv_color_hex(0x2F35DA),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui->ParamConfig_btn_save_label,
                             gui_assets_get_font_12(),
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_center(ui->ParamConfig_btn_save_label);

  // Status Label
  ui->ParamConfig_lbl_status = lv_label_create(ui->ParamConfig);
  lv_obj_set_pos(ui->ParamConfig_lbl_status, 10, 175);
  lv_label_set_text(ui->ParamConfig_lbl_status, "状态：默认参数已加载");
  lv_obj_set_style_text_color(ui->ParamConfig_lbl_status,
                              lv_color_hex(0xFFFFFF),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui->ParamConfig_lbl_status,
                             gui_assets_get_font_12(),
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_width(ui->ParamConfig_lbl_status, 300);

  // Keyboard
  ui->ParamConfig_kb = lv_keyboard_create(ui->ParamConfig);
  lv_obj_set_size(ui->ParamConfig_kb, 320, 120);
  lv_obj_align(ui->ParamConfig_kb, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_add_flag(ui->ParamConfig_kb, LV_OBJ_FLAG_HIDDEN);

  // Update layout
  lv_obj_update_layout(ui->ParamConfig);
  events_init_ParamConfig(ui);
}
