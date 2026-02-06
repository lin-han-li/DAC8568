#include "dac8568.h"
#include "gpio.h"

/* * 延时函数：根据实际示波器波形调整。
 * 如果波形依然偶尔乱码，请增大此循环次数 (如 200)。
 * 如果想追求更高频率且波形稳定，可尝试减小此值 (如 50)。
 */
static void DAC_Delay_Optimized(void) {
    for (volatile int i = 0; i < 100; i++) { 
        __NOP(); 
    }
}

void DAC8568_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  GPIO_InitStruct.Pin = DAC8568_CLK_Pin | DAC8568_DIN_Pin | DAC8568_SYNC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW; // 保持低速以防振铃
  
  HAL_GPIO_Init(DAC8568_CLK_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = DAC8568_LDAC_Pin;
  HAL_GPIO_Init(DAC8568_LDAC_GPIO_Port, &GPIO_InitStruct);
  
  GPIO_InitStruct.Pin = DAC8568_CLR_Pin;
  HAL_GPIO_Init(DAC8568_CLR_GPIO_Port, &GPIO_InitStruct);

  HAL_GPIO_WritePin(DAC8568_CLR_GPIO_Port, DAC8568_CLR_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(DAC8568_SYNC_GPIO_Port, DAC8568_SYNC_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(DAC8568_LDAC_GPIO_Port, DAC8568_LDAC_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(DAC8568_CLK_GPIO_Port, DAC8568_CLK_Pin, GPIO_PIN_RESET);
  
  HAL_Delay(50);
}

static void DAC8568_Transmit_SPI_32bit(uint32_t data) {
  uint32_t shift = 0x80000000;
  
  HAL_GPIO_WritePin(DAC8568_CLK_GPIO_Port, DAC8568_CLK_Pin, GPIO_PIN_RESET);
  DAC_Delay_Optimized();

  for (uint8_t i = 0; i < 32; i++) {
    if (data & shift) {
      HAL_GPIO_WritePin(DAC8568_DIN_GPIO_Port, DAC8568_DIN_Pin, GPIO_PIN_SET);
    } else {
      HAL_GPIO_WritePin(DAC8568_DIN_GPIO_Port, DAC8568_DIN_Pin, GPIO_PIN_RESET);
    }
    DAC_Delay_Optimized();

    HAL_GPIO_WritePin(DAC8568_CLK_GPIO_Port, DAC8568_CLK_Pin, GPIO_PIN_SET);
    DAC_Delay_Optimized();

    HAL_GPIO_WritePin(DAC8568_CLK_GPIO_Port, DAC8568_CLK_Pin, GPIO_PIN_RESET);
    DAC_Delay_Optimized();

    shift >>= 1;
  }
}

void DAC8568_Write_Register(uint32_t data) {
  __disable_irq(); // 关中断保护

  HAL_GPIO_WritePin(DAC8568_SYNC_GPIO_Port, DAC8568_SYNC_Pin, GPIO_PIN_RESET);
  DAC_Delay_Optimized();
  
  DAC8568_Transmit_SPI_32bit(data);
  
  DAC_Delay_Optimized();
  HAL_GPIO_WritePin(DAC8568_SYNC_GPIO_Port, DAC8568_SYNC_Pin, GPIO_PIN_SET);
  
  // 自动更新输出
  DAC_Delay_Optimized();
  HAL_GPIO_WritePin(DAC8568_LDAC_GPIO_Port, DAC8568_LDAC_Pin, GPIO_PIN_RESET);
  DAC_Delay_Optimized();
  HAL_GPIO_WritePin(DAC8568_LDAC_GPIO_Port, DAC8568_LDAC_Pin, GPIO_PIN_SET);
  
  __enable_irq(); // 恢复中断
}

void DAC8568_Internal_Reference_Config(bool state) {
  uint32_t write_data = ((uint32_t)Internal_Reference << 24) | (state ? 1 : 0);
  DAC8568_Write_Register(write_data);
}

void DAC8568_Reset(void) {
  uint32_t write_data = ((uint32_t)Software_Reset << 24) | 1;
  DAC8568_Write_Register(write_data);
}

/* 原始电压设置函数 */
void DAC8568_Set_Voltage(uint8_t channel, double voltage) {
#if DAC8568_VERSION
  double real_voltage = voltage / 4.0 + 2.5;
#else
  double real_voltage = voltage / 4.0 + 1.25;
#endif

  if(real_voltage < 0.0) real_voltage = 0.0;
  if(real_voltage > 5.0) real_voltage = 5.0; 

  uint16_t binary_voltage = (uint16_t)(real_voltage / 5.0 * 65535);
  
  // 调用下面的 Set_Code
  DAC8568_Set_Code(channel, binary_voltage);
}

/* 新增：极速设置代码函数 (跳过浮点运算) */
void DAC8568_Set_Code(uint8_t channel, uint16_t code) {
  if (channel > 0x07 && channel != All_DAC_Channels) return;

  // 0x03 = Write and Update Respective DAC
  uint32_t write_data = ((uint32_t)0x03 << 24) |
                        ((uint32_t)channel << 20) | 
                        (code << 4);     
  DAC8568_Write_Register(write_data);
}

void DAC8568_Set_Direct_Current(uint8_t channel, double voltage) {
  DAC8568_Set_Voltage(channel, voltage);
}

void DAC8568_Set_Channel_Wave(uint8_t channel, double *wave_data, uint16_t data_size) {
  for (uint16_t i = 0; i < data_size; i++) {
    DAC8568_Set_Voltage(channel, wave_data[i]);
  }
}
