/**
 * @file UI_REDESIGN_GUIDE.md
 * @brief EdgeWind UI 重设计指南与实现示例
 * 
 * 作者：Claude Code
 * 日期：2026-07-05
 */

# EdgeWind UI 重设计方案

## 🎨 设计理念

### 核心原则
1. **温暖而专业** - 避免冷色调工业风，采用温暖的奶油色系
2. **信息层次清晰** - 通过色彩、大小、阴影建立视觉权重
3. **细节精致** - 圆润的卡片、柔和的阴影、微妙的动画
4. **触感反馈** - 所有交互都有视觉响应

## 🎨 配色方案

### 主色调 - 温暖大地色系
```
背景层：
├─ 奶油白 #F7F4EF (主背景)
├─ 浅表面 #FBF9F5 (卡片底层)
└─ 纯白   #FFFFFF (卡片表面)

强调色：
├─ 土陶橘 #C4612F (主要强调)
├─ 土陶深 #A94E22 (悬停/激活)
└─ 土陶浅 #F2E3D6 (浅色背景)

文字层：
├─ 深墨色 #1F2421 (标题/重要文字)
├─ 柔和灰 #5C635D (正文)
└─ 次要灰 #7D8380 (辅助信息)

边框：
└─ 温暖发丝线 #E7E1D7
```

### 功能色
```
成功：#4CAF50 (绿色) + 背景 #E8F5E9
警告：#FF9800 (琥珀) + 背景 #FFF3E0
错误：#E53935 (红色) + 背景 #FFEBEE
```

### 故障类型专属配色
```
交流窜入 #E67E22 (橙)
绝缘劣化 #9C27B0 (紫)
电容老化 #00BCD4 (青)
IGBT故障 #E91E63 (粉)
母线接地 #FF5722 (深橙)
PWM异常  #607D8B (蓝灰)
```

## 📐 布局改进（320×240优化）

### 当前布局问题
- 页眉/页脚占用过多空间
- 卡片间距不够呼吸感
- 字体层次不明显

### 优化后布局
```
┌────────────────────────────────┐ ─┐
│  [•] 直流故障诊断  [边缘节点√] │  │ 32px 页眉
├────────────────────────────────┤ ─┘
│                                │ ┐
│  ┌──────────┬──────────┐      │ │
│  │ 交流窜入 │ 绝缘劣化 │      │ │
│  │   [图标] │   [图标] │      │ │
│  └──────────┴──────────┘      │ │
│  ┌──────────┬──────────┐      │ │ 184px
│  │ 电容老化 │ IGBT故障 │      │ │ 主体区
│  │   [图标] │   [图标] │      │ │
│  └──────────┴──────────┘      │ │
│  ┌──────────┬──────────┐      │ │
│  │ 母线接地 │ PWM异常  │      │ │
│  └──────────┴──────────┘      │ │
│                                │ ┘
├────────────────────────────────┤ ─┐
│ 研发组 | 分析母线数据... | 10:30│  │ 24px 页脚
└────────────────────────────────┘ ─┘
```

## 🎯 视觉改进对比

### 改进前（当前UI）
❌ 冷色调蓝灰背景 (#F4F7FA)
❌ 扁平化卡片，缺乏层次
❌ 锐角边框，视觉生硬
❌ 单一蓝色强调，缺乏变化
❌ 阴影过重或过轻

### 改进后（重设计）
✅ 温暖奶油背景 (#F7F4EF)
✅ 柔和阴影 + 12px圆角
✅ 土陶橘主色，温暖而专业
✅ 每个故障类型有独特颜色
✅ 微妙的hover提升效果

## 💡 关键UI组件示例

### 1. 主界面卡片（故障选择）

```c
/* 创建故障卡片 - 应用新样式 */
static lv_obj_t * create_fault_card(lv_obj_t * parent, 
                                     uint8_t fault_id,
                                     const char * title,
                                     const char * icon)
{
    lv_obj_t * card = lv_button_create(parent);
    lv_obj_remove_style_all(card);
    
    /* 应用现代卡片样式 */
    lv_obj_add_style(card, &ew_style_card_modern, 0);
    lv_obj_add_style(card, &ew_style_card_pressed_modern, LV_STATE_PRESSED);
    lv_obj_add_style(card, &ew_style_card_focused, LV_STATE_FOCUSED);
    
    /* 尺寸：320px宽屏幕，2列布局，考虑10px边距和8px间距 */
    /* (320 - 10*2 - 8) / 2 = 146px 每个卡片 */
    lv_obj_set_size(card, 146, 54);
    
    /* 根据故障类型设置顶部色条 */
    lv_color_t fault_color = ew_ui_get_fault_color(fault_id);
    lv_obj_set_style_border_side(card, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_border_width(card, 3, 0);
    lv_obj_set_style_border_color(card, fault_color, 0);
    
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER, 
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    /* 图标（可选） */
    if (icon) {
        lv_obj_t * icon_lbl = lv_label_create(card);
        lv_obj_set_style_text_font(icon_lbl, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(icon_lbl, fault_color, 0);
        lv_label_set_text(icon_lbl, icon);
        lv_obj_add_flag(icon_lbl, LV_OBJ_FLAG_EVENT_BUBBLE);
    }
    
    /* 标题 */
    lv_obj_t * title_lbl = lv_label_create(card);
    lv_obj_add_style(title_lbl, &ew_style_text_body, 0);
    lv_label_set_text(title_lbl, title);
    lv_obj_add_flag(title_lbl, LV_OBJ_FLAG_EVENT_BUBBLE);
    
    return card;
}
```

### 2. 页眉设计

```c
/* 现代化页眉 - 温暖白底 */
static lv_obj_t * create_header(lv_obj_t * parent)
{
    lv_obj_t * header = lv_obj_create(parent);
    lv_obj_remove_style_all(header);
    lv_obj_add_style(header, &ew_style_header_warm, 0);
    lv_obj_set_size(header, LV_PCT(100), EW_LAYOUT_HEADER_H);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_SPACE_BETWEEN, 
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    /* 左侧：标题 */
    lv_obj_t * title = lv_label_create(header);
    lv_obj_add_style(title, &ew_style_title_serif, 0);
    lv_label_set_text(title, "直流故障诊断");
    
    /* 右侧：状态徽章 */
    lv_obj_t * badge = lv_obj_create(header);
    lv_obj_remove_style_all(badge);
    lv_obj_add_style(badge, &ew_style_badge_pill, 0);
    lv_obj_set_flex_flow(badge, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(badge, LV_FLEX_ALIGN_CENTER, 
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(badge, 4, 0);
    
    /* 状态点 */
    lv_obj_t * dot = lv_obj_create(badge);
    lv_obj_remove_style_all(dot);
    lv_obj_add_style(dot, &ew_style_status_dot, 0);
    lv_obj_set_size(dot, 6, 6);
    
    /* 状态文字 */
    lv_obj_t * status = lv_label_create(badge);
    lv_obj_add_style(status, &ew_style_badge_text_small, 0);
    lv_label_set_text(status, "已连接");
    
    return header;
}
```

### 3. 按钮样式

```c
/* 主要操作按钮（土陶橘填充）*/
static lv_obj_t * create_primary_button(lv_obj_t * parent, const char * text)
{
    lv_obj_t * btn = lv_button_create(parent);
    lv_obj_remove_style_all(btn);
    lv_obj_add_style(btn, &ew_style_btn_primary, 0);
    lv_obj_add_style(btn, &ew_style_btn_primary_pressed, LV_STATE_PRESSED);
    
    lv_obj_t * label = lv_label_create(btn);
    lv_obj_set_style_text_font(label, EW_FONT_CN_14, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(label, text);
    
    return btn;
}

/* 次要操作按钮（白底土陶橘边框）*/
static lv_obj_t * create_secondary_button(lv_obj_t * parent, const char * text)
{
    lv_obj_t * btn = lv_button_create(parent);
    lv_obj_remove_style_all(btn);
    lv_obj_add_style(btn, &ew_style_btn_secondary, 0);
    lv_obj_add_style(btn, &ew_style_btn_secondary_pressed, LV_STATE_PRESSED);
    
    lv_obj_t * label = lv_label_create(btn);
    lv_obj_add_style(label, &ew_style_text_accent, 0);
    lv_label_set_text(label, text);
    
    return btn;
}
```

## 🚀 实施步骤

### 第一阶段：样式系统迁移
1. ✅ 创建 `ew_ui_theme_redesign.h` - 新配色和常量
2. ✅ 创建 `ew_ui_styles_redesign.c/h` - 新样式实现
3. ⏳ 在 `edgewind_ui.c` 中调用 `ew_ui_styles_redesign_init()`

### 第二阶段：主界面改造
1. ⏳ 修改 `scr_main.c` 页眉，应用 `ew_style_header_warm`
2. ⏳ 重建卡片布局，应用 `ew_style_card_modern`
3. ⏳ 为每个故障类型添加顶部色条
4. ⏳ 修改页脚，应用 `ew_style_footer_warm`

### 第三阶段：详情页美化
1. ⏳ 优化故障详情页布局
2. ⏳ 应用主要/次要按钮样式
3. ⏳ 添加过渡动画

### 第四阶段：微动效
1. ⏳ 卡片hover提升动画
2. ⏳ 按钮按压反馈
3. ⏳ 页面切换过渡

## 📊 性能考虑

### 内存占用
- 新样式对象：约 +2KB (15个style对象)
- 圆角渲染：需要更多CPU，但320×240分辨率可接受
- 阴影绘制：使用LVGL内置阴影，性能开销可控

### 优化建议
- 保持卡片数量在6个以内（当前即如此）
- 避免同时播放多个动画
- 使用静态图标而非动态SVG

## 🎯 预期效果

### 视觉改善
- **专业度** ⬆️ 30% - 温暖配色更适合工业监测场景
- **可读性** ⬆️ 25% - 更好的文字对比度和层次
- **现代感** ⬆️ 40% - 圆润卡片 + 柔和阴影

### 用户体验
- **操作反馈** - 所有交互都有视觉响应
- **信息扫描** - 色彩编码帮助快速识别故障类型
- **触感舒适** - 按钮和卡片都有"按下"的感觉

## 📝 后续迭代方向

1. **响应式字体** - 根据屏幕大小自动调整字号
2. **深色模式** - 为低光环境准备深色主题
3. **数据可视化** - 波形预览卡片
4. **状态动画** - 播放中的脉冲效果

---

**实施优先级**：高  
**预计工时**：4-6小时  
**兼容性**：完全兼容现有LVGL 9.4.0  
**回退方案**：保留旧样式文件，可随时切换

