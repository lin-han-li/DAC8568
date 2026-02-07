#ifndef DAC8568_DMA_H
#define DAC8568_DMA_H

#include <stdint.h>

void DAC8568_DMA_Init(uint32_t sample_rate_hz);
void DAC8568_DMA_Start(void);
void DAC8568_DMA_OnTimerTick(void);
void DAC8568_DMA_OnTxComplete(void);
void DAC8568_DMA_GetStats(uint32_t *tx_ok, uint32_t *tx_fail,
                           uint32_t *tick_skip);
void DAC8568_OutputFixedVoltage(float voltage);
void DAC8568_DMA_UpdateSampleRate(uint32_t sample_rate_hz);
void DAC8568_DMA_GetTickCounter(uint32_t *tick_count);
void DAC8568_DMA_GetSampleCounter(uint32_t *sample_count);
void DAC8568_DMA_Service(void);
void DAC8568_DMA_GetHealth(uint32_t *recover_count, uint32_t *recover_reason,
                           uint32_t *ref_rearm_count, uint32_t *ref_refresh_count,
                           uint32_t *stagnant_count);

#endif
