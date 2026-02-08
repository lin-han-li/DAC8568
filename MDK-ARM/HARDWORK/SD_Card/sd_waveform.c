#include "sd_waveform.h"

#include "SD.h"
#include "qspi_w25q256.h"
#include "sd_time.h"

#include "ff.h"

#include <stdio.h>
#include <string.h>

#define SD_DAC_WAVE_CHANNELS 4u
#define SD_DAC_WAVE_ERASE_UNIT 0x00010000u
#define SD_DAC_WAVE_MMAP_BASE 0x90000000u
#define SD_DAC_WAVE_IO_CHUNK 4096u

static uint32_t sd_dac_wave_checksum_update(uint32_t checksum, const uint8_t *data, uint32_t len)
{
	if (!data || len == 0u) {
		return checksum;
	}

	uint32_t value = checksum;
	for (uint32_t i = 0u; i < len; ++i) {
		value = (value * 16777619u) ^ data[i];
	}
	return value;
}

static bool sd_dac_wave_header_valid(const SD_DacWaveHeader_t *hdr)
{
	uint64_t total_bytes = 0u;
	uint32_t expected_data_bytes = 0u;

	if (!hdr) {
		return false;
	}
	if (hdr->magic != SD_DAC_WAVE_MAGIC || hdr->version != SD_DAC_WAVE_VERSION) {
		return false;
	}
	if (hdr->channel_count != SD_DAC_WAVE_CHANNELS) {
		return false;
	}
	if (hdr->sample_rate_hz == 0u || hdr->sample_count == 0u) {
		return false;
	}
	if (hdr->data_offset < sizeof(SD_DacWaveHeader_t)) {
		return false;
	}

	expected_data_bytes = hdr->sample_count * SD_DAC_WAVE_CHANNELS * (uint32_t)sizeof(uint16_t);
	if (hdr->data_bytes != expected_data_bytes) {
		return false;
	}

	total_bytes = (uint64_t)hdr->data_offset + (uint64_t)hdr->data_bytes;
	if (total_bytes > SD_DAC_QSPI_REGION_SIZE) {
		return false;
	}

	return true;
}

static void sd_dac_wave_info_from_header(const SD_DacWaveHeader_t *hdr, SD_DacWaveInfo_t *info)
{
	if (!hdr || !info) {
		return;
	}

	info->sample_rate_hz = hdr->sample_rate_hz;
	info->sample_count = hdr->sample_count;
	info->qspi_data_offset = SD_DAC_QSPI_BASE_OFFSET + hdr->data_offset;
	info->qspi_mmap_addr = SD_DAC_WAVE_MMAP_BASE + info->qspi_data_offset;
}

static bool sd_make_parent_dir(const char *path)
{
	if (!path) {
		return false;
	}
	char tmp[256];
	size_t len = strlen(path);
	if (len >= sizeof(tmp)) {
		return false;
	}
	memcpy(tmp, path, len + 1);
	char *slash = strrchr(tmp, '/');
	if (!slash) {
		return true;
	}
	if (slash == tmp) {
		return true;
	}
	*slash = '\0';
	return (SD_MkdirRecursive(tmp) == FR_OK);
}

bool SD_Wave_SaveBin(const char *name, const float *data, uint32_t len)
{
	return SD_Wave_SaveBinEx(name, data, len, NULL);
}

bool SD_Wave_SaveBinEx(const char *name, const float *data, uint32_t len, const SD_WaveMeta_t *meta)
{
	if (!name || !data || len == 0) {
		return false;
	}
	if (SD_Init() != FR_OK) {
		return false;
	}
	if (!sd_make_parent_dir(name)) {
		return false;
	}

	WaveFileHeader_t hdr = {0};
	hdr.magic = SD_WAVE_MAGIC;
	hdr.version = 1;
	hdr.timestamp = SD_Time_GetUnix();
	hdr.channel = meta ? meta->channel : 0;
	hdr.sample_rate = meta ? meta->sample_rate : 0;
	hdr.count = len;

	FIL fil;
	FRESULT res = f_open(&fil, name, FA_CREATE_ALWAYS | FA_WRITE);
	if (res != FR_OK) {
		return false;
	}
	UINT bw = 0;
	res = f_write(&fil, &hdr, sizeof(hdr), &bw);
	if (res != FR_OK || bw != sizeof(hdr)) {
		(void)f_close(&fil);
		return false;
	}
	res = f_write(&fil, data, sizeof(float) * len, &bw);
	(void)f_sync(&fil);
	(void)f_close(&fil);
	return (res == FR_OK && bw == sizeof(float) * len);
}

bool SD_Wave_LoadBin(const char *name, float *data, uint32_t *len)
{
	if (!name || !data || !len) {
		return false;
	}
	if (SD_Init() != FR_OK) {
		return false;
	}
	FIL fil;
	FRESULT res = f_open(&fil, name, FA_READ);
	if (res != FR_OK) {
		return false;
	}
	WaveFileHeader_t hdr = {0};
	UINT br = 0;
	res = f_read(&fil, &hdr, sizeof(hdr), &br);
	if (res != FR_OK || br != sizeof(hdr) || hdr.magic != SD_WAVE_MAGIC) {
		(void)f_close(&fil);
		return false;
	}
	uint32_t count = hdr.count;
	if (*len < count) {
		count = *len;
	}
	res = f_read(&fil, data, sizeof(float) * count, &br);
	(void)f_close(&fil);
	if (res != FR_OK) {
		return false;
	}
	*len = count;
	return true;
}

bool SD_Wave_SaveCSV(const char *name, const float *data, uint32_t len)
{
	if (!name || !data || len == 0) {
		return false;
	}
	if (SD_Init() != FR_OK) {
		return false;
	}
	if (!sd_make_parent_dir(name)) {
		return false;
	}
	FIL fil;
	FRESULT res = f_open(&fil, name, FA_CREATE_ALWAYS | FA_WRITE);
	if (res != FR_OK) {
		return false;
	}
	for (uint32_t i = 0; i < len; ++i) {
		char line[48];
		int n = snprintf(line, sizeof(line), "%lu,%.6f\r\n", (unsigned long)i, (double)data[i]);
		if (n <= 0) {
			continue;
		}
		UINT bw = 0;
		res = f_write(&fil, line, (UINT)n, &bw);
		if (res != FR_OK) {
			(void)f_close(&fil);
			return false;
		}
	}
	(void)f_sync(&fil);
	(void)f_close(&fil);
	return true;
}

bool SD_Wave_AutoSave(uint8_t channel, const float *data, uint32_t len, bool csv)
{
	if (!data || len == 0) {
		return false;
	}
	char date_path[64];
	if (!SD_Time_GetDatePath(date_path, sizeof(date_path), "0:/data")) {
		return false;
	}
	if (SD_MkdirRecursive(date_path) != FR_OK) {
		return false;
	}

	char ts[32];
	if (!SD_Time_GetTimestamp(ts, sizeof(ts))) {
		return false;
	}
	char file[128];
	const char *ext = csv ? "csv" : "bin";
	if (snprintf(file, sizeof(file), "%s/wave_ch%u_%s.%s",
	             date_path, (unsigned)channel, ts, ext) <= 0) {
		return false;
	}
	if (csv) {
		return SD_Wave_SaveCSV(file, data, len);
	}

	SD_WaveMeta_t meta;
	meta.channel = channel;
	meta.sample_rate = 0;
	meta.timestamp = SD_Time_GetUnix();
	return SD_Wave_SaveBinEx(file, data, len, &meta);
}

bool SD_Wave_SyncDacToQspi(const char *sd_path, SD_DacWaveInfo_t *info)
{
	FIL fil;
	FRESULT fres;
	FRESULT sd_res;
	UINT br = 0u;
	SD_DacWaveHeader_t hdr = {0};
	uint32_t checksum = 2166136261u;
	uint32_t written = 0u;
	uint32_t flash_total = 0u;
	uint32_t erase_end = 0u;
	static uint8_t io_buf[SD_DAC_WAVE_IO_CHUNK];

	if (!sd_path || !info) {
		return false;
	}
	memset(info, 0, sizeof(*info));

	sd_res = SD_Init();
	if (sd_res != FR_OK) {
		printf("[WAVE] SD init failed: %d\r\n", (int)sd_res);
		return false;
	}

	memset(&fil, 0, sizeof(fil));
	fres = f_open(&fil, sd_path, FA_READ);
	if (fres != FR_OK) {
		printf("[WAVE] open failed: %s (%d)\r\n", sd_path, (int)fres);
		return false;
	}

	fres = f_read(&fil, &hdr, sizeof(hdr), &br);
	if (fres != FR_OK || br != sizeof(hdr) || !sd_dac_wave_header_valid(&hdr)) {
		(void)f_close(&fil);
		printf("[WAVE] header invalid\r\n");
		return false;
	}

	flash_total = hdr.data_offset + hdr.data_bytes;
	erase_end = SD_DAC_QSPI_BASE_OFFSET + ((flash_total + (SD_DAC_WAVE_ERASE_UNIT - 1u)) & ~(SD_DAC_WAVE_ERASE_UNIT - 1u));

	if (QSPI_W25Qxx_Init() != QSPI_W25Qxx_OK) {
		(void)f_close(&fil);
		printf("[WAVE] QSPI init failed\r\n");
		return false;
	}
	(void)QSPI_W25Qxx_ExitMemoryMapped();

	for (uint32_t addr = SD_DAC_QSPI_BASE_OFFSET; addr < erase_end; addr += SD_DAC_WAVE_ERASE_UNIT) {
		if (QSPI_W25Qxx_BlockErase_64K(addr) != QSPI_W25Qxx_OK) {
			(void)f_close(&fil);
			printf("[WAVE] erase failed @0x%08lX\r\n", (unsigned long)addr);
			return false;
		}
	}

	if (QSPI_W25Qxx_WriteBuffer_Slow((uint8_t *)&hdr, SD_DAC_QSPI_BASE_OFFSET, sizeof(hdr)) != QSPI_W25Qxx_OK) {
		(void)f_close(&fil);
		printf("[WAVE] write header failed\r\n");
		return false;
	}

	fres = f_lseek(&fil, hdr.data_offset);
	if (fres != FR_OK) {
		(void)f_close(&fil);
		printf("[WAVE] seek data failed (%d)\r\n", (int)fres);
		return false;
	}

	while (written < hdr.data_bytes) {
		UINT req = (UINT)(hdr.data_bytes - written);
		if (req > (UINT)sizeof(io_buf)) {
			req = (UINT)sizeof(io_buf);
		}

		fres = f_read(&fil, io_buf, req, &br);
		if (fres != FR_OK || br == 0u) {
			(void)f_close(&fil);
			printf("[WAVE] read data failed (%d)\r\n", (int)fres);
			return false;
		}

		if (QSPI_W25Qxx_WriteBuffer_Slow(io_buf, SD_DAC_QSPI_BASE_OFFSET + hdr.data_offset + written, br) != QSPI_W25Qxx_OK) {
			(void)f_close(&fil);
			printf("[WAVE] write data failed @%lu\r\n", (unsigned long)written);
			return false;
		}

		checksum = sd_dac_wave_checksum_update(checksum, io_buf, br);
		written += br;
	}
	(void)f_close(&fil);

	if (written != hdr.data_bytes || checksum != hdr.checksum) {
		printf("[WAVE] checksum mismatch exp=0x%08lX got=0x%08lX\r\n",
		       (unsigned long)hdr.checksum, (unsigned long)checksum);
		return false;
	}

	{
		SD_DacWaveHeader_t check_hdr = {0};
		if (QSPI_W25Qxx_ReadBuffer_Slow((uint8_t *)&check_hdr, SD_DAC_QSPI_BASE_OFFSET, sizeof(check_hdr)) != QSPI_W25Qxx_OK) {
			printf("[WAVE] readback header failed\r\n");
			return false;
		}
		if (memcmp(&check_hdr, &hdr, sizeof(hdr)) != 0) {
			printf("[WAVE] readback header mismatch\r\n");
			return false;
		}
	}

	if (QSPI_W25Qxx_EnterMemoryMapped() != QSPI_W25Qxx_OK) {
		printf("[WAVE] enter memory-mapped failed\r\n");
		return false;
	}

	sd_dac_wave_info_from_header(&hdr, info);
	printf("[WAVE] sync ok: sps=%lu count=%lu addr=0x%08lX\r\n",
	       (unsigned long)info->sample_rate_hz,
	       (unsigned long)info->sample_count,
	       (unsigned long)info->qspi_mmap_addr);
	return true;
}

bool SD_Wave_LoadDacInfoFromQspi(SD_DacWaveInfo_t *info)
{
	SD_DacWaveHeader_t hdr = {0};

	if (!info) {
		return false;
	}
	memset(info, 0, sizeof(*info));

	if (QSPI_W25Qxx_Init() != QSPI_W25Qxx_OK) {
		return false;
	}
	(void)QSPI_W25Qxx_ExitMemoryMapped();

	if (QSPI_W25Qxx_ReadBuffer_Slow((uint8_t *)&hdr, SD_DAC_QSPI_BASE_OFFSET, sizeof(hdr)) != QSPI_W25Qxx_OK) {
		return false;
	}
	if (!sd_dac_wave_header_valid(&hdr)) {
		return false;
	}
	if (QSPI_W25Qxx_EnterMemoryMapped() != QSPI_W25Qxx_OK) {
		return false;
	}

	sd_dac_wave_info_from_header(&hdr, info);
	return true;
}
