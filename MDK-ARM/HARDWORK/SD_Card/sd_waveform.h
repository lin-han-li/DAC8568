#ifndef SD_WAVEFORM_H
#define SD_WAVEFORM_H

#include <stdbool.h>
#include <stdint.h>

#define SD_WAVE_MAGIC 0x57415645u /* "WAVE" */
#define SD_DAC_WAVE_MAGIC 0x44384357u /* "D8CW" */
#define SD_DAC_WAVE_VERSION 1u

/*
 * Reserved QSPI region for DAC waveform direct-read playback.
 *
 * W25Q256 total size: 32MB (0x02000000).
 * Partition:
 *  - 0x00000000 ~ 0x003FFFFF (4MB): reserved for future use
 *  - 0x00400000 ~ 0x01FFFFFF (28MB): DAC waveform storage (7 x 4MB)
 *
 * UI assets sync to QSPI is disabled in this project; the reserved 4MB stays free.
 */
#define SD_DAC_QSPI_BASE_OFFSET 0x00400000u
#define SD_DAC_QSPI_REGION_SIZE 0x01C00000u
#define SD_DAC_QSPI_PARTITION_COUNT 7u
#define SD_DAC_QSPI_PARTITION_SIZE  0x00400000u
#define SD_DAC_QSPI_PART0_OFFSET (SD_DAC_QSPI_BASE_OFFSET + 0x000000u) /* normal */
#define SD_DAC_QSPI_PART1_OFFSET (SD_DAC_QSPI_BASE_OFFSET + 0x400000u) /* ac coupling */
#define SD_DAC_QSPI_PART2_OFFSET (SD_DAC_QSPI_BASE_OFFSET + 0x800000u) /* bus ground */
#define SD_DAC_QSPI_PART3_OFFSET (SD_DAC_QSPI_BASE_OFFSET + 0xC00000u) /* insulation */
#define SD_DAC_QSPI_PART4_OFFSET (SD_DAC_QSPI_BASE_OFFSET + 0x1000000u) /* cap aging */
#define SD_DAC_QSPI_PART5_OFFSET (SD_DAC_QSPI_BASE_OFFSET + 0x1400000u) /* pwm abnormal */
#define SD_DAC_QSPI_PART6_OFFSET (SD_DAC_QSPI_BASE_OFFSET + 0x1800000u) /* igbt fault */

typedef enum {
	SD_DAC_WAVE_PART_NORMAL = 0u,         /* 正常波形 */
	SD_DAC_WAVE_PART_AC_COUPLING = 1u,    /* 交流窜入 */
	SD_DAC_WAVE_PART_BUS_GROUND = 2u,     /* 母线接地 */
	SD_DAC_WAVE_PART_INSULATION = 3u,     /* 绝缘劣化 */
	SD_DAC_WAVE_PART_CAP_AGING = 4u,      /* 电容老化 */
	SD_DAC_WAVE_PART_PWM_ABNORMAL = 5u,   /* PWM异常 */
	SD_DAC_WAVE_PART_IGBT_FAULT = 6u      /* IGBT故障 */
} SD_DacWavePartition_t;

typedef struct {
	uint32_t magic;
	uint32_t version;
	uint32_t sample_rate_hz;
	uint32_t sample_count;
	uint32_t channel_count;
	uint32_t data_offset;
	uint32_t data_bytes;
	uint32_t checksum;
} SD_DacWaveHeader_t;

typedef struct {
	uint32_t sample_rate_hz;
	uint32_t sample_count;
	uint32_t qspi_data_offset;
	uint32_t qspi_mmap_addr;
	uint32_t partition_id;
} SD_DacWaveInfo_t;

typedef struct {
	uint32_t magic;
	uint32_t version;
	uint32_t timestamp;
	uint32_t channel;
	uint32_t sample_rate;
	uint32_t count;
} WaveFileHeader_t;

typedef struct {
	uint32_t channel;
	uint32_t sample_rate;
	uint32_t timestamp;
} SD_WaveMeta_t;

bool SD_Wave_SaveBin(const char *name, const float *data, uint32_t len);
bool SD_Wave_SaveBinEx(const char *name, const float *data, uint32_t len, const SD_WaveMeta_t *meta);
bool SD_Wave_LoadBin(const char *name, float *data, uint32_t *len);
bool SD_Wave_SaveCSV(const char *name, const float *data, uint32_t len);
bool SD_Wave_AutoSave(uint8_t channel, const float *data, uint32_t len, bool csv);
bool SD_Wave_SyncDacToQspi(const char *sd_path, SD_DacWaveInfo_t *info);
bool SD_Wave_LoadDacInfoFromQspi(SD_DacWaveInfo_t *info);
bool SD_Wave_SyncDacToQspiPartition(const char *sd_path, SD_DacWavePartition_t partition, SD_DacWaveInfo_t *info);
bool SD_Wave_LoadDacInfoFromQspiPartition(SD_DacWavePartition_t partition, SD_DacWaveInfo_t *info);
uint32_t SD_Wave_GetPartitionBaseOffset(SD_DacWavePartition_t partition);
const char *SD_Wave_GetPartitionName(SD_DacWavePartition_t partition);

#endif /* SD_WAVEFORM_H */
