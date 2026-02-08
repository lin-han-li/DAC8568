#ifndef DAC8568_DMA_H
#define DAC8568_DMA_H

#include <stdint.h>

typedef enum {
  DAC8568_SOURCE_LUT = 0,
  DAC8568_SOURCE_QSPI = 1
} DAC8568_SourceMode_t;

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
int32_t DAC8568_DMA_UseQspiWave(uint32_t qspi_mmap_addr, uint32_t sample_count,
                                uint32_t sample_rate_hz);
void DAC8568_DMA_UseBuiltInWave(void);
DAC8568_SourceMode_t DAC8568_DMA_GetSourceMode(void);

/* Optional waveform source: prebuilt 32-bit SPI frames in memory-mapped QSPI. */
void DAC8568_DMA_SetWaveFrames(const uint32_t *mm_frames, uint32_t sample_count);
/* Optional waveform source: 4x uint16 DAC codes per sample in memory-mapped QSPI. */
void DAC8568_DMA_SetWaveCodes16(const uint16_t *mm_codes, uint32_t sample_count);
void DAC8568_DMA_DisableWaveFrames(void);

#endif
