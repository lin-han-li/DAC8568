#include "dac_wave_sync.h"

#include "SD.h"
#include "qspi_w25q256.h"

#include "ff.h"

#include <stdio.h>
#include <string.h>

extern int printf(const char *format, ...);

#define DAC_WAVE_MAGIC 0x44414357u /* "DACW" (new: 32-bit frames) */
#define DAC_WAVE_VERSION 1u
#define DAC_WAVE_LEGACY_MAGIC 0x44384357u /* "D8CW" (legacy: 4x uint16 code) */
#define DAC_WAVE_LEGACY_VERSION 1u

/* Reserve a 1MB window near the end of W25Q256 for wave data. */
#define DAC_WAVE_QSPI_BASE_OFF 0x01F00000u
#define DAC_WAVE_QSPI_WINDOW_BYTES 0x00100000u
#define DAC_WAVE_QSPI_DATA_OFF 0x40u

#define DAC_WAVE_WORDS_PER_SAMPLE 4u

typedef struct {
  uint32_t magic;
  uint32_t version;
  uint32_t header_bytes;
  uint32_t sample_rate;
  uint32_t sample_count;
  uint32_t words_per_sample;
  uint32_t crc32;
  uint32_t reserved[9];
} DAC_WaveFileHeader_t; /* 64 bytes */

typedef struct {
  uint32_t magic;
  uint32_t version;
  uint32_t sample_rate_hz;
  uint32_t sample_count;
  uint32_t channel_count;
  uint32_t data_offset;
  uint32_t data_bytes;
  uint32_t checksum;
} DAC_WaveLegacyHeader_t; /* 32 bytes */

static uint32_t crc32_update(uint32_t crc, const uint8_t *data, uint32_t len) {
  crc = ~crc;
  for (uint32_t i = 0; i < len; i++) {
    crc ^= (uint32_t)data[i];
    for (uint32_t bit = 0; bit < 8u; bit++) {
      uint32_t mask = (uint32_t)-(int32_t)(crc & 1u);
      crc = (crc >> 1) ^ (0xEDB88320u & mask);
    }
  }
  return ~crc;
}

static uint32_t fnv1a_legacy_update(uint32_t checksum, const uint8_t *data, uint32_t len) {
  uint32_t value = checksum;
  for (uint32_t i = 0u; i < len; ++i) {
    value = (value * 16777619u) ^ data[i];
  }
  return value;
}

static uint32_t align_up(uint32_t value, uint32_t align) {
  return (value + (align - 1u)) & ~(align - 1u);
}

bool DAC_Wave_SyncFromSDToW25Q(const char *path, DAC_WaveInfo_t *out_info) {
  if (!path || !out_info) {
    return false;
  }
  memset(out_info, 0, sizeof(*out_info));

  if (sizeof(DAC_WaveFileHeader_t) != DAC_WAVE_QSPI_DATA_OFF) {
    printf("[WAVE] internal error: header size mismatch\r\n");
    return false;
  }

  if (SD_Init() != FR_OK) {
    printf("[WAVE] SD init failed\r\n");
    return false;
  }

  FIL fil;
  FRESULT fr = f_open(&fil, path, FA_READ);
  if (fr != FR_OK) {
    printf("[WAVE] open failed: %s (%d)\r\n", path, (int)fr);
    return false;
  }

  DAC_WaveFileHeader_t hdr = {0};
  DAC_WaveLegacyHeader_t hdr_legacy = {0};
  UINT br = 0;
  fr = f_read(&fil, &hdr, sizeof(hdr), &br);
  if (fr != FR_OK || br != sizeof(hdr)) {
    printf("[WAVE] read header failed: %d br=%u\r\n", (int)fr, (unsigned)br);
    (void)f_close(&fil);
    return false;
  }

  uint32_t wave_format = DAC_WAVE_FORMAT_NONE;
  uint32_t data_offset = DAC_WAVE_QSPI_DATA_OFF;
  uint32_t data_bytes = 0u;
  uint32_t sample_rate = 0u;
  uint32_t sample_count = 0u;
  uint32_t expected_crc = 0u;

  if (hdr.magic == DAC_WAVE_MAGIC &&
      hdr.version == DAC_WAVE_VERSION &&
      hdr.header_bytes == sizeof(hdr) &&
      hdr.words_per_sample == DAC_WAVE_WORDS_PER_SAMPLE &&
      hdr.sample_rate != 0u &&
      hdr.sample_count != 0u) {
    wave_format = DAC_WAVE_FORMAT_FRAME32;
    data_offset = hdr.header_bytes;
    data_bytes = hdr.sample_count * hdr.words_per_sample * (uint32_t)sizeof(uint32_t);
    sample_rate = hdr.sample_rate;
    sample_count = hdr.sample_count;
    expected_crc = hdr.crc32;
  } else {
    memcpy(&hdr_legacy, &hdr, sizeof(hdr_legacy));
    uint32_t expect_bytes = hdr_legacy.sample_count * 4u * (uint32_t)sizeof(uint16_t);
    if (hdr_legacy.magic == DAC_WAVE_LEGACY_MAGIC &&
        hdr_legacy.version == DAC_WAVE_LEGACY_VERSION &&
        hdr_legacy.sample_rate_hz != 0u &&
        hdr_legacy.sample_count != 0u &&
        hdr_legacy.channel_count == 4u &&
        hdr_legacy.data_offset >= sizeof(hdr_legacy) &&
        hdr_legacy.data_bytes == expect_bytes) {
      wave_format = DAC_WAVE_FORMAT_CODE16x4;
      data_offset = hdr_legacy.data_offset;
      data_bytes = hdr_legacy.data_bytes;
      sample_rate = hdr_legacy.sample_rate_hz;
      sample_count = hdr_legacy.sample_count;
      expected_crc = hdr_legacy.checksum;
    }
  }

  if (wave_format == DAC_WAVE_FORMAT_NONE) {
    printf("[WAVE] bad header: magic=0x%08lX ver=%lu hdr=%lu sps=%lu cnt=%lu wps=%lu\r\n",
           (unsigned long)hdr.magic,
           (unsigned long)hdr.version,
           (unsigned long)hdr.header_bytes,
           (unsigned long)hdr.sample_rate,
           (unsigned long)hdr.sample_count,
           (unsigned long)hdr.words_per_sample);
    (void)f_close(&fil);
    return false;
  }

  if ((data_offset + data_bytes) > DAC_WAVE_QSPI_WINDOW_BYTES) {
    printf("[WAVE] too large: %lu bytes (window %lu)\r\n",
           (unsigned long)(data_offset + data_bytes),
           (unsigned long)DAC_WAVE_QSPI_WINDOW_BYTES);
    (void)f_close(&fil);
    return false;
  }

  FSIZE_t file_size = f_size(&fil);
  if (file_size < (FSIZE_t)(data_offset + data_bytes)) {
    printf("[WAVE] file too small: %lu need>=%lu\r\n",
           (unsigned long)file_size,
           (unsigned long)(data_offset + data_bytes));
    (void)f_close(&fil);
    return false;
  }

  int8_t qret = QSPI_W25Qxx_Init();
  uint32_t jedec = QSPI_W25Qxx_ReadID();
  printf("[W25Q256] JEDEC raw=%02X %02X %02X => 0x%06lX\r\n",
         (unsigned)((jedec >> 16) & 0xFFu),
         (unsigned)((jedec >> 8) & 0xFFu),
         (unsigned)(jedec & 0xFFu),
         (unsigned long)jedec);
  if (qret != QSPI_W25Qxx_OK) {
    printf("[WAVE] QSPI init failed: %d\r\n", (int)qret);
    (void)f_close(&fil);
    return false;
  }

  /* If flash already contains the exact same header, skip re-program. */
  uint8_t existing_header[DAC_WAVE_QSPI_DATA_OFF] = {0};
  qret = QSPI_W25Qxx_ReadBuffer(existing_header, DAC_WAVE_QSPI_BASE_OFF, DAC_WAVE_QSPI_DATA_OFF);
  if (qret == QSPI_W25Qxx_OK && memcmp(existing_header, &hdr, DAC_WAVE_QSPI_DATA_OFF) == 0) {
    qret = QSPI_W25Qxx_EnterMemoryMapped();
    if (qret != QSPI_W25Qxx_OK) {
      printf("[WAVE] enter memory-mapped failed: %d\r\n", (int)qret);
      (void)f_close(&fil);
      return false;
    }

    uint32_t mm_addr = (uint32_t)W25Qxx_Mem_Addr + (DAC_WAVE_QSPI_BASE_OFF + data_offset);
    uintptr_t inv_start = (uintptr_t)mm_addr & ~(uintptr_t)31u;
    uintptr_t inv_end = ((uintptr_t)mm_addr + data_bytes + 31u) & ~(uintptr_t)31u;
    SCB_InvalidateDCache_by_Addr((uint32_t *)inv_start, (int32_t)(inv_end - inv_start));

    out_info->sample_rate = sample_rate;
    out_info->sample_count = sample_count;
    out_info->qspi_mm_addr = mm_addr;
    out_info->crc32 = expected_crc;
    out_info->format = wave_format;
    printf("[WAVE] already synced (skip program)\r\n");
    (void)f_close(&fil);
    return true;
  }

  /* Rewind to payload and program flash. */
  (void)f_lseek(&fil, (FSIZE_t)data_offset);

  (void)QSPI_W25Qxx_ExitMemoryMapped();

  uint32_t qspi_write_base = DAC_WAVE_QSPI_BASE_OFF;
  uint32_t qspi_write_end = DAC_WAVE_QSPI_BASE_OFF + data_offset + data_bytes;
  qspi_write_end = align_up(qspi_write_end, 0x1000u);

  for (uint32_t addr = qspi_write_base; addr < qspi_write_end; addr += 0x1000u) {
    qret = QSPI_W25Qxx_SectorErase(addr);
    if (qret != QSPI_W25Qxx_OK) {
      printf("[WAVE] erase failed @0x%08lX ret=%d\r\n", (unsigned long)addr, (int)qret);
      (void)f_close(&fil);
      return false;
    }
  }

  qret = QSPI_W25Qxx_WriteBuffer((uint8_t *)&hdr, DAC_WAVE_QSPI_BASE_OFF, DAC_WAVE_QSPI_DATA_OFF);
  if (qret != QSPI_W25Qxx_OK) {
    printf("[WAVE] write header failed: %d\r\n", (int)qret);
    (void)f_close(&fil);
    return false;
  }

  static uint8_t io_buf[4096];
  uint32_t crc = (wave_format == DAC_WAVE_FORMAT_CODE16x4) ? 2166136261u : 0u;
  uint32_t remaining = data_bytes;
  uint32_t flash_addr = DAC_WAVE_QSPI_BASE_OFF + data_offset;

  while (remaining > 0u) {
    UINT chunk = (remaining > sizeof(io_buf)) ? (UINT)sizeof(io_buf) : (UINT)remaining;
    br = 0;
    fr = f_read(&fil, io_buf, chunk, &br);
    if (fr != FR_OK || br != chunk) {
      printf("[WAVE] read payload failed: %d br=%u\r\n", (int)fr, (unsigned)br);
      (void)f_close(&fil);
      return false;
    }

    if (wave_format == DAC_WAVE_FORMAT_CODE16x4) {
      crc = fnv1a_legacy_update(crc, io_buf, (uint32_t)chunk);
    } else {
      crc = crc32_update(crc, io_buf, (uint32_t)chunk);
    }

    qret = QSPI_W25Qxx_WriteBuffer(io_buf, flash_addr, (uint32_t)chunk);
    if (qret != QSPI_W25Qxx_OK) {
      printf("[WAVE] write payload failed @0x%08lX ret=%d\r\n",
             (unsigned long)flash_addr, (int)qret);
      (void)f_close(&fil);
      return false;
    }

    flash_addr += (uint32_t)chunk;
    remaining -= (uint32_t)chunk;
  }

  (void)f_close(&fil);

  if (crc != expected_crc) {
    printf("[WAVE] CRC mismatch: got=0x%08lX expect=0x%08lX\r\n",
           (unsigned long)crc, (unsigned long)expected_crc);
    return false;
  }

  qret = QSPI_W25Qxx_EnterMemoryMapped();
  if (qret != QSPI_W25Qxx_OK) {
    printf("[WAVE] enter memory-mapped failed: %d\r\n", (int)qret);
    return false;
  }

  uint32_t mm_addr = (uint32_t)W25Qxx_Mem_Addr + (DAC_WAVE_QSPI_BASE_OFF + data_offset);
  uintptr_t inv_start = (uintptr_t)mm_addr & ~(uintptr_t)31u;
  uintptr_t inv_end = ((uintptr_t)mm_addr + data_bytes + 31u) & ~(uintptr_t)31u;
  SCB_InvalidateDCache_by_Addr((uint32_t *)inv_start, (int32_t)(inv_end - inv_start));

  out_info->sample_rate = sample_rate;
  out_info->sample_count = sample_count;
  out_info->qspi_mm_addr = mm_addr;
  out_info->crc32 = expected_crc;
  out_info->format = wave_format;

  printf("[WAVE] sync ok: fmt=%lu sps=%lu count=%lu addr=0x%08lX\r\n",
         (unsigned long)wave_format,
         (unsigned long)sample_rate,
         (unsigned long)sample_count,
         (unsigned long)mm_addr);
  return true;
}
