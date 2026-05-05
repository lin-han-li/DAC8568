#ifndef DAC_WAVE_SYNC_H
#define DAC_WAVE_SYNC_H

#include <stdbool.h>
#include <stdint.h>

#define DAC_WAVE_FORMAT_NONE 0u
#define DAC_WAVE_FORMAT_FRAME32 1u
#define DAC_WAVE_FORMAT_CODE16x4 2u

typedef struct {
  uint32_t sample_rate;
  uint32_t sample_count;
  uint32_t qspi_mm_addr; /* Absolute memory-mapped address (0x9000xxxx). */
  uint32_t crc32;
  uint32_t format;
} DAC_WaveInfo_t;

bool DAC_Wave_SyncFromSDToW25Q(const char *path, DAC_WaveInfo_t *out_info);

#endif /* DAC_WAVE_SYNC_H */
