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
#define SD_DAC_WAVE_CHECKSUM_SEED 2166136261u

static bool sd_dac_wave_partition_valid(SD_DacWavePartition_t partition)
{
	return ((uint32_t)partition < SD_DAC_QSPI_PARTITION_COUNT);
}

uint32_t SD_Wave_GetPartitionBaseOffset(SD_DacWavePartition_t partition)
{
	if (!sd_dac_wave_partition_valid(partition)) {
		return SD_DAC_QSPI_BASE_OFFSET;
	}
	return SD_DAC_QSPI_BASE_OFFSET + ((uint32_t)partition * SD_DAC_QSPI_PARTITION_SIZE);
}

const char *SD_Wave_GetPartitionName(SD_DacWavePartition_t partition)
{
	switch (partition) {
	case SD_DAC_WAVE_PART_NORMAL: return "normal";
	case SD_DAC_WAVE_PART_AC_COUPLING: return "ac_coupling";
	case SD_DAC_WAVE_PART_INSULATION: return "insulation";
	case SD_DAC_WAVE_PART_CAP_AGING: return "cap_aging";
	case SD_DAC_WAVE_PART_IGBT_FAULT: return "igbt_fault";
	case SD_DAC_WAVE_PART_BUS_GROUND: return "bus_ground";
	case SD_DAC_WAVE_PART_PWM_ABNORMAL: return "pwm_abnormal";
	default: return "unknown";
	}
}

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

static bool sd_dac_wave_sd_payload_checksums(FIL *fil, const SD_DacWaveHeader_t *hdr,
                                             uint32_t *canonical_checksum_out,
                                             uint32_t *zero_seed_checksum_out,
                                             uint8_t *io_buf,
                                             uint32_t io_buf_len)
{
	uint32_t canonical_checksum = SD_DAC_WAVE_CHECKSUM_SEED;
	uint32_t zero_seed_checksum = 0u;
	uint32_t done = 0u;
	FRESULT fres;
	UINT br = 0u;

	if (!fil || !hdr || !canonical_checksum_out || !zero_seed_checksum_out || !io_buf || io_buf_len == 0u) {
		return false;
	}

	fres = f_lseek(fil, hdr->data_offset);
	if (fres != FR_OK) {
		printf("[WAVE] SD precheck seek failed (%d)\r\n", (int)fres);
		return false;
	}

	while (done < hdr->data_bytes) {
		UINT req = (UINT)(hdr->data_bytes - done);
		if (req > (UINT)io_buf_len) {
			req = (UINT)io_buf_len;
		}

		fres = f_read(fil, io_buf, req, &br);
		if (fres != FR_OK || br != req) {
			printf("[WAVE] SD precheck read failed (%d, br=%lu req=%lu)\r\n",
			       (int)fres,
			       (unsigned long)br,
			       (unsigned long)req);
			return false;
		}

		canonical_checksum = sd_dac_wave_checksum_update(canonical_checksum, io_buf, br);
		zero_seed_checksum = sd_dac_wave_checksum_update(zero_seed_checksum, io_buf, br);
		done += br;
	}

	*canonical_checksum_out = canonical_checksum;
	*zero_seed_checksum_out = zero_seed_checksum;
	return true;
}

static bool sd_dac_wave_qspi_checksum(uint32_t qspi_addr, uint32_t data_bytes,
                                      uint32_t *checksum_out, uint8_t *io_buf,
                                      uint32_t io_buf_len)
{
	uint32_t checksum = SD_DAC_WAVE_CHECKSUM_SEED;
	uint32_t done = 0u;

	if (!checksum_out || !io_buf || io_buf_len == 0u) {
		return false;
	}

	while (done < data_bytes) {
		uint32_t chunk = data_bytes - done;
		if (chunk > io_buf_len) {
			chunk = io_buf_len;
		}

		if (QSPI_W25Qxx_ReadBuffer_Slow(io_buf, qspi_addr + done, chunk) != QSPI_W25Qxx_OK) {
			return false;
		}
		checksum = sd_dac_wave_checksum_update(checksum, io_buf, chunk);
		done += chunk;
	}

	*checksum_out = checksum;
	return true;
}

static bool sd_dac_wave_qspi_payload_valid(uint32_t partition_base,
                                           const SD_DacWaveHeader_t *hdr,
                                           uint32_t *checksum_out,
                                           uint8_t *io_buf,
                                           uint32_t io_buf_len)
{
	uint32_t checksum = 0u;
	uint32_t zero_seed_checksum = 0u;

	if (!hdr) {
		return false;
	}
	if (!sd_dac_wave_qspi_checksum(partition_base + hdr->data_offset,
	                               hdr->data_bytes,
	                               &checksum,
	                               io_buf,
	                               io_buf_len)) {
		return false;
	}
	if (checksum_out != NULL) {
		*checksum_out = checksum;
	}
	if (checksum == hdr->checksum) {
		return true;
	}

	/* Compatibility for AI-training HIL packages generated before 2026-05-07.
	 * Those files used a zero-seed checksum in the header. Playback keeps the
	 * canonical FNV offset checksum when writing new QSPI headers, but accepting
	 * the old seed lets existing SD/QSPI contents be recovered without data loss.
	 */
	zero_seed_checksum = 0u;
	for (uint32_t done = 0u; done < hdr->data_bytes;) {
		uint32_t chunk = hdr->data_bytes - done;
		if (chunk > io_buf_len) {
			chunk = io_buf_len;
		}
		if (QSPI_W25Qxx_ReadBuffer_Slow(io_buf, partition_base + hdr->data_offset + done, chunk) != QSPI_W25Qxx_OK) {
			return false;
		}
		zero_seed_checksum = sd_dac_wave_checksum_update(zero_seed_checksum, io_buf, chunk);
		done += chunk;
	}
	if (checksum_out != NULL) {
		*checksum_out = zero_seed_checksum;
	}
	return (zero_seed_checksum == hdr->checksum);
}

static bool sd_dac_wave_header_valid(const SD_DacWaveHeader_t *hdr, uint32_t max_region_bytes)
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
	if (total_bytes > max_region_bytes) {
		return false;
	}

	return true;
}

static void sd_dac_wave_info_from_header(const SD_DacWaveHeader_t *hdr,
                                         uint32_t partition_base,
                                         SD_DacWavePartition_t partition,
                                         SD_DacWaveInfo_t *info)
{
	if (!hdr || !info) {
		return;
	}

	info->sample_rate_hz = hdr->sample_rate_hz;
	info->sample_count = hdr->sample_count;
	info->qspi_data_offset = partition_base + hdr->data_offset;
	info->qspi_mmap_addr = SD_DAC_WAVE_MMAP_BASE + info->qspi_data_offset;
	info->partition_id = (uint32_t)partition;
	info->checksum = hdr->checksum;
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

bool SD_Wave_SyncDacToQspiPartition(const char *sd_path, SD_DacWavePartition_t partition, SD_DacWaveInfo_t *info)
{
	FIL fil;
	FRESULT fres;
	FRESULT sd_res;
	UINT br = 0u;
	SD_DacWaveHeader_t hdr = {0};
	SD_DacWaveHeader_t write_hdr = {0};
	SD_DacWaveHeader_t qspi_hdr = {0};
	uint32_t checksum = SD_DAC_WAVE_CHECKSUM_SEED;
	uint32_t sd_checksum = 0u;
	uint32_t sd_checksum_zero_seed = 0u;
	uint32_t qspi_checksum = 0u;
	uint32_t written = 0u;
	uint32_t flash_total = 0u;
	uint32_t erase_end = 0u;
	uint32_t partition_base = 0u;
	static uint8_t io_buf[SD_DAC_WAVE_IO_CHUNK];

	if (!sd_path || !info) {
		return false;
	}
	if (!sd_dac_wave_partition_valid(partition)) {
		printf("[WAVE] invalid partition: %lu\r\n", (unsigned long)partition);
		return false;
	}
	memset(info, 0, sizeof(*info));
	partition_base = SD_Wave_GetPartitionBaseOffset(partition);
	printf("[WAVE] sync start: part=%s(%lu) path=%s\r\n",
	       SD_Wave_GetPartitionName(partition),
	       (unsigned long)partition,
	       sd_path);

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
	if (fres != FR_OK || br != sizeof(hdr) || !sd_dac_wave_header_valid(&hdr, SD_DAC_QSPI_PARTITION_SIZE)) {
		(void)f_close(&fil);
		printf("[WAVE] header invalid\r\n");
		return false;
	}

	if (!sd_dac_wave_sd_payload_checksums(&fil, &hdr, &sd_checksum, &sd_checksum_zero_seed, io_buf, sizeof(io_buf)) ||
	    ((sd_checksum != hdr.checksum) && (sd_checksum_zero_seed != hdr.checksum))) {
		(void)f_close(&fil);
		printf("[WAVE] SD precheck checksum mismatch: part=%s header=0x%08lX canonical=0x%08lX zero=0x%08lX, skip erase\r\n",
		       SD_Wave_GetPartitionName(partition),
		       (unsigned long)hdr.checksum,
		       (unsigned long)sd_checksum,
		       (unsigned long)sd_checksum_zero_seed);
		return false;
	}
	write_hdr = hdr;
	if (sd_checksum_zero_seed == hdr.checksum && sd_checksum != hdr.checksum) {
		write_hdr.checksum = sd_checksum;
		printf("[WAVE] SD precheck ok: part=%s checksum=0x%08lX zero-seed header normalized to 0x%08lX\r\n",
		       SD_Wave_GetPartitionName(partition),
		       (unsigned long)sd_checksum_zero_seed,
		       (unsigned long)sd_checksum);
	} else {
		printf("[WAVE] SD precheck ok: part=%s checksum=0x%08lX\r\n",
	       SD_Wave_GetPartitionName(partition),
	       (unsigned long)sd_checksum);
	}

	flash_total = write_hdr.data_offset + write_hdr.data_bytes;
	erase_end = partition_base + ((flash_total + (SD_DAC_WAVE_ERASE_UNIT - 1u)) & ~(SD_DAC_WAVE_ERASE_UNIT - 1u));

	if (QSPI_W25Qxx_BeginCommandMode() != QSPI_W25Qxx_OK) {
		(void)f_close(&fil);
		printf("[WAVE] QSPI busy, stop DAC QSPI playback before sync\r\n");
		return false;
	}

	/* Ensure QSPI is not left in memory-mapped mode from previous partition. */
	(void)QSPI_W25Qxx_ExitMemoryMapped();
	if (QSPI_W25Qxx_Init() != QSPI_W25Qxx_OK) {
		QSPI_W25Qxx_EndCommandMode();
		(void)f_close(&fil);
		printf("[WAVE] QSPI init failed\r\n");
		return false;
	}
	(void)QSPI_W25Qxx_ExitMemoryMapped();

	if (QSPI_W25Qxx_ReadBuffer_Slow((uint8_t *)&qspi_hdr, partition_base, sizeof(qspi_hdr)) == QSPI_W25Qxx_OK &&
	    sd_dac_wave_header_valid(&qspi_hdr, SD_DAC_QSPI_PARTITION_SIZE) &&
	    memcmp(&qspi_hdr, &write_hdr, sizeof(write_hdr)) == 0) {
		if (sd_dac_wave_qspi_payload_valid(partition_base, &qspi_hdr, &qspi_checksum, io_buf, sizeof(io_buf))) {
			if (QSPI_W25Qxx_EnterMemoryMapped() != QSPI_W25Qxx_OK) {
				QSPI_W25Qxx_EndCommandMode();
				(void)f_close(&fil);
				printf("[WAVE] enter memory-mapped failed after skip\r\n");
				return false;
			}
			QSPI_W25Qxx_EndCommandMode();
			(void)f_close(&fil);

			sd_dac_wave_info_from_header(&write_hdr, partition_base, partition, info);
			printf("[WAVE] sync skip/already current: part=%s(%lu) checksum=0x%08lX addr=0x%08lX\r\n",
			       SD_Wave_GetPartitionName(partition),
			       (unsigned long)partition,
			       (unsigned long)qspi_checksum,
			       (unsigned long)info->qspi_mmap_addr);
			return true;
		}
		printf("[WAVE] QSPI payload stale: part=%s exp=0x%08lX got=0x%08lX\r\n",
		       SD_Wave_GetPartitionName(partition),
		       (unsigned long)write_hdr.checksum,
		       (unsigned long)qspi_checksum);
	}

	for (uint32_t addr = partition_base; addr < erase_end; addr += SD_DAC_WAVE_ERASE_UNIT) {
		if (QSPI_W25Qxx_BlockErase_64K(addr) != QSPI_W25Qxx_OK) {
			QSPI_W25Qxx_EndCommandMode();
			(void)f_close(&fil);
			printf("[WAVE] erase failed @0x%08lX\r\n", (unsigned long)addr);
			return false;
		}
	}

	fres = f_lseek(&fil, write_hdr.data_offset);
	if (fres != FR_OK) {
		QSPI_W25Qxx_EndCommandMode();
		(void)f_close(&fil);
		printf("[WAVE] seek data failed (%d)\r\n", (int)fres);
		return false;
	}

	while (written < write_hdr.data_bytes) {
		UINT req = (UINT)(write_hdr.data_bytes - written);
		if (req > (UINT)sizeof(io_buf)) {
			req = (UINT)sizeof(io_buf);
		}

		fres = f_read(&fil, io_buf, req, &br);
		if (fres != FR_OK || br != req) {
			QSPI_W25Qxx_EndCommandMode();
			(void)f_close(&fil);
			printf("[WAVE] read data failed (%d, br=%lu req=%lu)\r\n",
			       (int)fres,
			       (unsigned long)br,
			       (unsigned long)req);
			return false;
		}

		if (QSPI_W25Qxx_WriteBuffer_Slow(io_buf, partition_base + write_hdr.data_offset + written, br) != QSPI_W25Qxx_OK) {
			QSPI_W25Qxx_EndCommandMode();
			(void)f_close(&fil);
			printf("[WAVE] write data failed @%lu\r\n", (unsigned long)written);
			return false;
		}

		checksum = sd_dac_wave_checksum_update(checksum, io_buf, br);
		written += br;
	}
	(void)f_close(&fil);

	if (written != write_hdr.data_bytes || checksum != write_hdr.checksum) {
		QSPI_W25Qxx_EndCommandMode();
		printf("[WAVE] checksum mismatch exp=0x%08lX got=0x%08lX\r\n",
		       (unsigned long)write_hdr.checksum, (unsigned long)checksum);
		return false;
	}

	if (!sd_dac_wave_qspi_payload_valid(partition_base, &write_hdr, &qspi_checksum, io_buf, sizeof(io_buf))) {
		QSPI_W25Qxx_EndCommandMode();
		printf("[WAVE] QSPI verify data failed: part=%s exp=0x%08lX got=0x%08lX\r\n",
		       SD_Wave_GetPartitionName(partition),
		       (unsigned long)write_hdr.checksum,
		       (unsigned long)qspi_checksum);
		return false;
	}

	if (QSPI_W25Qxx_WriteBuffer_Slow((uint8_t *)&write_hdr, partition_base, sizeof(write_hdr)) != QSPI_W25Qxx_OK) {
		QSPI_W25Qxx_EndCommandMode();
		printf("[WAVE] write header failed\r\n");
		return false;
	}

	{
		SD_DacWaveHeader_t check_hdr = {0};
		if (QSPI_W25Qxx_ReadBuffer_Slow((uint8_t *)&check_hdr, partition_base, sizeof(check_hdr)) != QSPI_W25Qxx_OK) {
			QSPI_W25Qxx_EndCommandMode();
			printf("[WAVE] readback header failed\r\n");
			return false;
		}
		if (memcmp(&check_hdr, &write_hdr, sizeof(write_hdr)) != 0) {
			QSPI_W25Qxx_EndCommandMode();
			printf("[WAVE] readback header mismatch\r\n");
			return false;
		}
	}

	if (QSPI_W25Qxx_EnterMemoryMapped() != QSPI_W25Qxx_OK) {
		QSPI_W25Qxx_EndCommandMode();
		printf("[WAVE] enter memory-mapped failed\r\n");
		return false;
	}
	QSPI_W25Qxx_EndCommandMode();

	sd_dac_wave_info_from_header(&write_hdr, partition_base, partition, info);
	printf("[WAVE] sync ok: part=%s(%lu) sps=%lu count=%lu checksum=0x%08lX addr=0x%08lX\r\n",
	       SD_Wave_GetPartitionName(partition),
	       (unsigned long)partition,
	       (unsigned long)info->sample_rate_hz,
	       (unsigned long)info->sample_count,
	       (unsigned long)info->checksum,
	       (unsigned long)info->qspi_mmap_addr);
	return true;
}

bool SD_Wave_LoadDacInfoFromQspiPartition(SD_DacWavePartition_t partition, SD_DacWaveInfo_t *info)
{
	SD_DacWaveHeader_t hdr = {0};
	uint32_t qspi_checksum = 0u;
	uint32_t partition_base = 0u;
	static uint8_t io_buf[SD_DAC_WAVE_IO_CHUNK];

	if (!info) {
		return false;
	}
	if (!sd_dac_wave_partition_valid(partition)) {
		return false;
	}
	memset(info, 0, sizeof(*info));
	partition_base = SD_Wave_GetPartitionBaseOffset(partition);

	if (QSPI_W25Qxx_BeginCommandMode() != QSPI_W25Qxx_OK) {
		return false;
	}
	(void)QSPI_W25Qxx_ExitMemoryMapped();
	if (QSPI_W25Qxx_Init() != QSPI_W25Qxx_OK) {
		QSPI_W25Qxx_EndCommandMode();
		return false;
	}
	(void)QSPI_W25Qxx_ExitMemoryMapped();

	if (QSPI_W25Qxx_ReadBuffer_Slow((uint8_t *)&hdr, partition_base, sizeof(hdr)) != QSPI_W25Qxx_OK) {
		QSPI_W25Qxx_EndCommandMode();
		return false;
	}
	if (!sd_dac_wave_header_valid(&hdr, SD_DAC_QSPI_PARTITION_SIZE)) {
		QSPI_W25Qxx_EndCommandMode();
		printf("[WAVE] QSPI header invalid: part=%s\r\n", SD_Wave_GetPartitionName(partition));
		return false;
	}
	if (!sd_dac_wave_qspi_payload_valid(partition_base, &hdr, &qspi_checksum, io_buf, sizeof(io_buf))) {
		QSPI_W25Qxx_EndCommandMode();
		printf("[WAVE] QSPI checksum invalid: part=%s exp=0x%08lX got=0x%08lX\r\n",
		       SD_Wave_GetPartitionName(partition),
		       (unsigned long)hdr.checksum,
		       (unsigned long)qspi_checksum);
		return false;
	}
	if (QSPI_W25Qxx_EnterMemoryMapped() != QSPI_W25Qxx_OK) {
		QSPI_W25Qxx_EndCommandMode();
		return false;
	}
	QSPI_W25Qxx_EndCommandMode();

	sd_dac_wave_info_from_header(&hdr, partition_base, partition, info);
	return true;
}

bool SD_Wave_SyncDacToQspi(const char *sd_path, SD_DacWaveInfo_t *info)
{
	return SD_Wave_SyncDacToQspiPartition(sd_path, SD_DAC_WAVE_PART_NORMAL, info);
}

bool SD_Wave_LoadDacInfoFromQspi(SD_DacWaveInfo_t *info)
{
	return SD_Wave_LoadDacInfoFromQspiPartition(SD_DAC_WAVE_PART_NORMAL, info);
}
