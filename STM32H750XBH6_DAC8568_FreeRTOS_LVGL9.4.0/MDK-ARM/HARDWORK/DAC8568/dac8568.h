#ifndef __DAC8568_H__
#define __DAC8568_H__

#include "main.h"
#include <stdbool.h>
#include <math.h> // 需要用到 sin 函数生成表

/* ================= 配置区域 ================= */
#define DAC8568_VERSION 1 

#if DAC8568_VERSION
  #define MAX_VOLTAGE 10.0
  #define MIN_VOLTAGE -10.0
#else
  #define MAX_VOLTAGE 5.0
  #define MIN_VOLTAGE -5.0
#endif

#define VREF 2500.0 

/* 寄存器命令定义 */
typedef enum {
  Write_DAC_Input_Reg             = 0x00,
  Update_DAC_Reg                  = 0x01,
  Write_Update_All_DAC_Input_Reg  = 0x02,
  Write_Update_Respective_DAC_Reg = 0x03, 
  Power_Down_Command              = 0x04,
  Software_Reset                  = 0x07,
  Internal_Reference              = 0x08,
} DAC8568_Command_t;

/* 通道定义 */
typedef enum {
  DAC_Channel_A = 0x00,
  DAC_Channel_B,
  DAC_Channel_C,
  DAC_Channel_D,
  DAC_Channel_E,
  DAC_Channel_F,
  DAC_Channel_G,
  DAC_Channel_H,
  All_DAC_Channels = 0x0F,
} DAC8568_Channel_t;

/* 函数声明 */
void DAC8568_Init(void);
void DAC8568_Reset(void);
void DAC8568_Internal_Reference_Config(bool state);
void DAC8568_Set_Voltage(uint8_t channel, double voltage);
void DAC8568_Set_Direct_Current(uint8_t channel, double voltage);
void DAC8568_Set_Channel_Wave(uint8_t channel, double *wave_data, uint16_t data_size);

/* 新增：极速模式，直接发送原始数据 */
void DAC8568_Set_Code(uint8_t channel, uint16_t code);

/* 底层调试 */
void DAC8568_Write_Register(uint32_t data);

#endif
