#ifndef DAC8568_AUX4_H
#define DAC8568_AUX4_H

#include <stdbool.h>
#include <stdint.h>

#define DAC8568_AUX4_COUNT 4u

typedef struct {
  uint8_t loaded;
  uint8_t using_default;
  uint8_t source;
  uint8_t active_file_index;
  uint32_t active_item_count;
  uint32_t inject_count;
  uint32_t parse_error_count;
  uint32_t last_item_index;
  uint32_t generation;
  uint32_t payload_checksum;
  char active_file[24];
  float values[DAC8568_AUX4_COUNT];
  float volts[DAC8568_AUX4_COUNT];
  float range_lo[DAC8568_AUX4_COUNT];
  float range_hi[DAC8568_AUX4_COUNT];
  float default_values[DAC8568_AUX4_COUNT];
} DAC8568_Aux4Status_t;

void DAC8568_Aux4_Init(void);
bool DAC8568_Aux4_SyncFromSdToQspi(const char *path);
bool DAC8568_Aux4_LoadFromQspi(void);
bool DAC8568_Aux4_LoadFromSd(const char *path);
bool DAC8568_Aux4_VerifyWave(uint8_t partition_id,
                             uint32_t sample_rate_hz,
                             uint32_t sample_count,
                             uint32_t checksum);
void DAC8568_Aux4_SetActiveFile(const char *bin_path_or_name, bool reset_index);
bool DAC8568_Aux4_ConsumeDue(uint8_t file_index,
                             uint32_t sample_index,
                             uint32_t samples_elapsed,
                             float values[DAC8568_AUX4_COUNT],
                             float volts[DAC8568_AUX4_COUNT]);
void DAC8568_Aux4_GetStatus(DAC8568_Aux4Status_t *status);

#endif
