#include "DAC8568/dac8568_aux4.h"

#include "SD.h"
#include "ff.h"
#include "qspi_w25q256.h"
#include "stm32h7xx_hal.h"

#include <stdio.h>
#include <string.h>

#define AUX4_FILE_COUNT 7u
#define AUX4_MAX_ITEMS_PER_FILE 32u
#define AUX4_SLOT_SIZE (64u * 1024u)
#define AUX4_SLOT0_OFFSET 0x00000000u
#define AUX4_SLOT1_OFFSET 0x00010000u
#define AUX4_MAGIC0 'E'
#define AUX4_MAGIC1 'W'
#define AUX4_MAGIC2 'A'
#define AUX4_MAGIC3 'U'
#define AUX4_MAGIC4 'X'
#define AUX4_MAGIC5 '4'
#define AUX4_VERSION 1u
#define AUX4_HEADER_BYTES 256u
#define AUX4_FILE_ENTRY_BYTES 96u
#define AUX4_ITEM_BYTES 16u
#define AUX4_DEFAULT_SAMPLES_PER_ITEM 16384u
#define AUX4_DAC_SAMPLE_RATE_HZ 102400u
#define AUX4_MONITOR_SAMPLE_RATE_HZ 25600u
#define AUX4_MONITOR_WINDOW_POINTS 4096u
#define AUX4_FNV1A_OFFSET 2166136261u
#define AUX4_FNV1A_PRIME 16777619u
#define AUX4_RANGE_EPSILON 0.0001f
#define AUX4_RANGE_ABS_LIMIT 10000000.0f

#define AUX4_SOURCE_DEFAULT 0u
#define AUX4_SOURCE_QSPI 1u
#define AUX4_SOURCE_SD 2u

typedef struct __attribute__((packed)) {
  char magic[8];
  uint16_t version;
  uint16_t header_bytes;
  uint32_t total_bytes;
  uint32_t payload_fnv1a32;
  uint32_t generation;
  uint32_t dac_sample_rate_hz;
  uint32_t monitor_sample_rate_hz;
  uint32_t monitor_window_points;
  uint32_t dac_samples_per_aux;
  uint32_t aux_count;
  uint32_t file_count;
  uint32_t file_table_offset;
  uint32_t file_entry_bytes;
  uint32_t item_table_offset;
  uint32_t item_stride_bytes;
  float aux_lo[DAC8568_AUX4_COUNT];
  float aux_hi[DAC8568_AUX4_COUNT];
  float aux_default[DAC8568_AUX4_COUNT];
  char dataset_version[96];
  uint8_t reserved[48];
} aux4_bin_header_t;

typedef struct __attribute__((packed)) {
  char bin_name[32];
  uint32_t partition_id;
  uint32_t class_id;
  uint32_t d8cw_sample_rate_hz;
  uint32_t d8cw_sample_count;
  uint32_t d8cw_checksum;
  uint32_t item_start;
  uint32_t item_count;
  uint32_t samples_per_item;
  uint32_t flags;
  uint32_t records_fnv1a32;
  uint8_t reserved[24];
} aux4_file_entry_t;

typedef struct {
  const char *file_name;
  uint8_t loaded;
  uint8_t verified;
  uint8_t reserved0;
  uint8_t reserved1;
  uint32_t item_count;
  uint32_t samples_per_item;
  uint32_t d8cw_sample_rate_hz;
  uint32_t d8cw_sample_count;
  uint32_t d8cw_checksum;
  uint32_t last_emitted_item;
  float values[AUX4_MAX_ITEMS_PER_FILE][DAC8568_AUX4_COUNT];
} aux4_schedule_t;

typedef char aux4_header_size_check[(sizeof(aux4_bin_header_t) == AUX4_HEADER_BYTES) ? 1 : -1];
typedef char aux4_file_entry_size_check[(sizeof(aux4_file_entry_t) == AUX4_FILE_ENTRY_BYTES) ? 1 : -1];

static const char * const s_aux4_files[AUX4_FILE_COUNT] = {
  "normal.bin",
  "ac_coupling.bin",
  "insulation.bin",
  "cap_aging.bin",
  "igbt_fault.bin",
  "bus_ground.bin",
  "pwm_abnormal.bin",
};

static const float s_aux4_legacy_lo[DAC8568_AUX4_COUNT] = {
  20.0f,
  18.0f,
  8.0f,
  8.0f,
};

static const float s_aux4_legacy_hi[DAC8568_AUX4_COUNT] = {
  125.0f,
  115.0f,
  98.0f,
  110.0f,
};

static const float s_aux4_legacy_default_values[DAC8568_AUX4_COUNT] = {
  72.5f,
  66.5f,
  53.0f,
  59.0f,
};

static float s_aux4_lo[DAC8568_AUX4_COUNT] = {
  20.0f,
  18.0f,
  8.0f,
  8.0f,
};

static float s_aux4_hi[DAC8568_AUX4_COUNT] = {
  125.0f,
  115.0f,
  98.0f,
  110.0f,
};

static float s_aux4_default_values[DAC8568_AUX4_COUNT] = {
  72.5f,
  66.5f,
  53.0f,
  59.0f,
};

__attribute__((section(".axi_sram"), aligned(32))) static uint8_t s_aux4_blob[AUX4_SLOT_SIZE];

static aux4_schedule_t s_schedules[AUX4_FILE_COUNT];
static uint8_t s_loaded = 0u;
static uint8_t s_source = AUX4_SOURCE_DEFAULT;
static uint8_t s_using_default = 1u;
static uint8_t s_active_file_index = 0u;
static uint8_t s_pending_initial = 1u;
static uint32_t s_default_samples_accum = 0u;
static uint32_t s_inject_count = 0u;
static uint32_t s_parse_error_count = 0u;
static uint32_t s_last_item_index = 0u;
static uint32_t s_generation = 0u;
static uint32_t s_payload_checksum = 0u;
static char s_active_file_name[24] = "normal.bin";
static float s_current_values[DAC8568_AUX4_COUNT];
static float s_current_volts[DAC8568_AUX4_COUNT];

static uint32_t aux4_fnv1a32(const uint8_t *data, uint32_t len)
{
  uint32_t hash = AUX4_FNV1A_OFFSET;
  for (uint32_t i = 0u; i < len; i++) {
    hash ^= data[i];
    hash *= AUX4_FNV1A_PRIME;
  }
  return hash;
}

static float aux4_clampf(float value, float lo, float hi)
{
  if (value < lo) {
    return lo;
  }
  if (value > hi) {
    return hi;
  }
  return value;
}

static float aux4_value_to_volt(uint32_t channel, float value)
{
  const float lo = s_aux4_lo[channel];
  const float hi = s_aux4_hi[channel];
  const float clamped = aux4_clampf(value, lo, hi);
  return 0.5f + (4.0f * (clamped - lo) / (hi - lo));
}

static void aux4_copy4(float dst[DAC8568_AUX4_COUNT],
                       const float src[DAC8568_AUX4_COUNT])
{
  for (uint32_t i = 0u; i < DAC8568_AUX4_COUNT; i++) {
    dst[i] = src[i];
  }
}

static void aux4_make_volts(const float values[DAC8568_AUX4_COUNT],
                            float volts[DAC8568_AUX4_COUNT])
{
  for (uint32_t i = 0u; i < DAC8568_AUX4_COUNT; i++) {
    volts[i] = aux4_value_to_volt(i, values[i]);
  }
}

static void aux4_set_current(const float values[DAC8568_AUX4_COUNT],
                             uint32_t item_index)
{
  aux4_copy4(s_current_values, values);
  aux4_make_volts(values, s_current_volts);
  s_last_item_index = item_index;
}

static const char *aux4_basename(const char *path_or_name)
{
  const char *base = path_or_name;

  if (base == NULL) {
    return "normal.bin";
  }
  for (const char *p = path_or_name; *p != '\0'; p++) {
    if ((*p == '/') || (*p == '\\') || (*p == ':')) {
      base = p + 1;
    }
  }
  return (*base != '\0') ? base : "normal.bin";
}

static int32_t aux4_file_index(const char *path_or_name)
{
  const char *name = aux4_basename(path_or_name);

  for (uint32_t i = 0u; i < AUX4_FILE_COUNT; i++) {
    if (strcmp(name, s_aux4_files[i]) == 0) {
      return (int32_t)i;
    }
  }
  return -1;
}

static void aux4_reset_schedules(void)
{
  for (uint32_t i = 0u; i < AUX4_FILE_COUNT; i++) {
    s_schedules[i].file_name = s_aux4_files[i];
    s_schedules[i].loaded = 0u;
    s_schedules[i].verified = 0u;
    s_schedules[i].item_count = 0u;
    s_schedules[i].samples_per_item = AUX4_DEFAULT_SAMPLES_PER_ITEM;
    s_schedules[i].d8cw_sample_rate_hz = 0u;
    s_schedules[i].d8cw_sample_count = 0u;
    s_schedules[i].d8cw_checksum = 0u;
    s_schedules[i].last_emitted_item = 0xFFFFFFFFu;
    memset(s_schedules[i].values, 0, sizeof(s_schedules[i].values));
  }
}

static void aux4_set_default_active(void)
{
  s_using_default = 1u;
  s_active_file_index = 0u;
  s_pending_initial = 1u;
  s_default_samples_accum = 0u;
  (void)snprintf(s_active_file_name, sizeof(s_active_file_name), "%s", "normal.bin");
  aux4_set_current(s_aux4_default_values, 0u);
}

static bool aux4_float_valid(float value)
{
  return (value == value) &&
         (value > -AUX4_RANGE_ABS_LIMIT) &&
         (value < AUX4_RANGE_ABS_LIMIT);
}

static bool aux4_float_matches(float got, float expected)
{
  float diff = got - expected;

  if (diff < 0.0f) {
    diff = -diff;
  }
  return diff <= AUX4_RANGE_EPSILON;
}

static bool aux4_range_pair_matches(float got_lo,
                                    float got_hi,
                                    float expected_lo,
                                    float expected_hi)
{
  return aux4_float_matches(got_lo, expected_lo) &&
         aux4_float_matches(got_hi, expected_hi);
}

static void aux4_restore_legacy_ranges(void)
{
  for (uint32_t i = 0u; i < DAC8568_AUX4_COUNT; i++) {
    s_aux4_lo[i] = s_aux4_legacy_lo[i];
    s_aux4_hi[i] = s_aux4_legacy_hi[i];
    s_aux4_default_values[i] = s_aux4_legacy_default_values[i];
  }
}

static void aux4_apply_header_ranges(const aux4_bin_header_t *hdr)
{
  aux4_copy4(s_aux4_lo, hdr->aux_lo);
  aux4_copy4(s_aux4_hi, hdr->aux_hi);
  aux4_copy4(s_aux4_default_values, hdr->aux_default);
}

static bool aux4_ranges_supported(const aux4_bin_header_t *hdr)
{
  bool ch3_legacy;
  bool ch3_riso;

  if (hdr == NULL) {
    return false;
  }

  for (uint32_t i = 0u; i < DAC8568_AUX4_COUNT; i++) {
    if (!aux4_float_valid(hdr->aux_lo[i]) ||
        !aux4_float_valid(hdr->aux_hi[i]) ||
        !aux4_float_valid(hdr->aux_default[i]) ||
        (hdr->aux_hi[i] <= hdr->aux_lo[i]) ||
        (hdr->aux_default[i] < hdr->aux_lo[i]) ||
        (hdr->aux_default[i] > hdr->aux_hi[i])) {
      return false;
    }
  }

  for (uint32_t i = 0u; i < 3u; i++) {
    if (!aux4_range_pair_matches(hdr->aux_lo[i],
                                 hdr->aux_hi[i],
                                 s_aux4_legacy_lo[i],
                                 s_aux4_legacy_hi[i])) {
      return false;
    }
  }

  ch3_legacy = aux4_range_pair_matches(hdr->aux_lo[3],
                                       hdr->aux_hi[3],
                                       8.0f,
                                       110.0f);
  ch3_riso = aux4_range_pair_matches(hdr->aux_lo[3],
                                     hdr->aux_hi[3],
                                     20.0f,
                                     8000.0f);
  return ch3_legacy || ch3_riso;
}

static bool aux4_header_basic_valid(const aux4_bin_header_t *hdr)
{
  if (hdr == NULL) {
    return false;
  }
  if ((hdr->magic[0] != AUX4_MAGIC0) ||
      (hdr->magic[1] != AUX4_MAGIC1) ||
      (hdr->magic[2] != AUX4_MAGIC2) ||
      (hdr->magic[3] != AUX4_MAGIC3) ||
      (hdr->magic[4] != AUX4_MAGIC4) ||
      (hdr->magic[5] != AUX4_MAGIC5) ||
      (hdr->magic[6] != '\0') ||
      (hdr->magic[7] != '\0')) {
    return false;
  }
  if ((hdr->version != AUX4_VERSION) ||
      (hdr->header_bytes != AUX4_HEADER_BYTES) ||
      (hdr->total_bytes < AUX4_HEADER_BYTES) ||
      (hdr->total_bytes > AUX4_SLOT_SIZE)) {
    return false;
  }
  return true;
}

static bool aux4_blob_valid(const uint8_t *blob, uint32_t len, aux4_bin_header_t *hdr_out)
{
  aux4_bin_header_t hdr;
  uint32_t table_end;
  uint32_t payload_len;
  uint32_t payload_hash;

  if ((blob == NULL) || (len < sizeof(hdr))) {
    return false;
  }
  memcpy(&hdr, blob, sizeof(hdr));
  if (!aux4_header_basic_valid(&hdr)) {
    return false;
  }
  if (hdr.total_bytes > len) {
    return false;
  }
  if ((hdr.dac_sample_rate_hz != AUX4_DAC_SAMPLE_RATE_HZ) ||
      (hdr.monitor_sample_rate_hz != AUX4_MONITOR_SAMPLE_RATE_HZ) ||
      (hdr.monitor_window_points != AUX4_MONITOR_WINDOW_POINTS) ||
      (hdr.dac_samples_per_aux != AUX4_DEFAULT_SAMPLES_PER_ITEM) ||
      (hdr.aux_count != DAC8568_AUX4_COUNT) ||
      (hdr.file_count != AUX4_FILE_COUNT) ||
      (hdr.file_table_offset != AUX4_HEADER_BYTES) ||
      (hdr.file_entry_bytes != AUX4_FILE_ENTRY_BYTES) ||
      (hdr.item_stride_bytes != AUX4_ITEM_BYTES)) {
    return false;
  }
  if (!aux4_ranges_supported(&hdr)) {
    return false;
  }

  table_end = hdr.file_table_offset + (hdr.file_count * hdr.file_entry_bytes);
  if ((table_end < hdr.file_table_offset) ||
      (hdr.item_table_offset < table_end) ||
      (hdr.item_table_offset > hdr.total_bytes)) {
    return false;
  }
  payload_len = hdr.total_bytes - hdr.file_table_offset;
  payload_hash = aux4_fnv1a32(&blob[hdr.file_table_offset], payload_len);
  if (payload_hash != hdr.payload_fnv1a32) {
    return false;
  }
  if (hdr_out != NULL) {
    *hdr_out = hdr;
  }
  return true;
}

static bool aux4_load_blob_into_ram(const uint8_t *blob, uint32_t len, uint8_t source)
{
  aux4_bin_header_t hdr;
  uint32_t loaded_count = 0u;
  char dataset_version[sizeof(hdr.dataset_version) + 1u];

  if (!aux4_blob_valid(blob, len, &hdr)) {
    s_parse_error_count++;
    aux4_set_default_active();
    return false;
  }

  aux4_reset_schedules();
  for (uint32_t i = 0u; i < hdr.file_count; i++) {
    aux4_file_entry_t entry;
    uint32_t entry_offset = hdr.file_table_offset + (i * hdr.file_entry_bytes);
    uint32_t records_offset;
    uint32_t records_bytes;
    int32_t file_idx;

    memcpy(&entry, &blob[entry_offset], sizeof(entry));
    entry.bin_name[sizeof(entry.bin_name) - 1u] = '\0';
    file_idx = aux4_file_index(entry.bin_name);
    if ((file_idx < 0) ||
        (entry.partition_id >= AUX4_FILE_COUNT) ||
        ((uint32_t)file_idx != entry.partition_id) ||
        (entry.class_id >= AUX4_FILE_COUNT) ||
        (entry.class_id != entry.partition_id) ||
        (entry.item_count == 0u) ||
        (entry.item_count > AUX4_MAX_ITEMS_PER_FILE) ||
        (entry.samples_per_item != hdr.dac_samples_per_aux) ||
        (entry.flags != 0u)) {
      s_parse_error_count++;
      continue;
    }

    records_offset = hdr.item_table_offset + (entry.item_start * hdr.item_stride_bytes);
    records_bytes = entry.item_count * hdr.item_stride_bytes;
    if ((records_offset < hdr.item_table_offset) ||
        (records_bytes > hdr.total_bytes) ||
        ((records_offset + records_bytes) > hdr.total_bytes)) {
      s_parse_error_count++;
      continue;
    }
    if (aux4_fnv1a32(&blob[records_offset], records_bytes) != entry.records_fnv1a32) {
      s_parse_error_count++;
      continue;
    }

    aux4_schedule_t *schedule = &s_schedules[file_idx];
    schedule->loaded = 1u;
    schedule->verified = 0u;
    schedule->item_count = entry.item_count;
    schedule->samples_per_item = entry.samples_per_item;
    schedule->d8cw_sample_rate_hz = entry.d8cw_sample_rate_hz;
    schedule->d8cw_sample_count = entry.d8cw_sample_count;
    schedule->d8cw_checksum = entry.d8cw_checksum;
    schedule->last_emitted_item = 0xFFFFFFFFu;
    for (uint32_t item = 0u; item < entry.item_count; item++) {
      memcpy(schedule->values[item],
             &blob[records_offset + (item * hdr.item_stride_bytes)],
             DAC8568_AUX4_COUNT * sizeof(float));
    }
    loaded_count++;
  }

  if (loaded_count == 0u) {
    s_parse_error_count++;
    aux4_set_default_active();
    return false;
  }

  s_loaded = 1u;
  s_source = source;
  s_generation = hdr.generation;
  s_payload_checksum = hdr.payload_fnv1a32;
  aux4_apply_header_ranges(&hdr);
  DAC8568_Aux4_SetActiveFile("normal.bin", true);
  memcpy(dataset_version, hdr.dataset_version, sizeof(hdr.dataset_version));
  dataset_version[sizeof(hdr.dataset_version)] = '\0';
  printf("[AUX4] loaded source=%u generation=%lu files=%lu checksum=0x%08lX range_hi_x1000=[%ld,%ld,%ld,%ld] default_x1000=[%ld,%ld,%ld,%ld] dataset=%s\r\n",
         (unsigned)source,
         (unsigned long)hdr.generation,
         (unsigned long)loaded_count,
         (unsigned long)hdr.payload_fnv1a32,
         (long)(hdr.aux_hi[0] * 1000.0f + 0.5f),
         (long)(hdr.aux_hi[1] * 1000.0f + 0.5f),
         (long)(hdr.aux_hi[2] * 1000.0f + 0.5f),
         (long)(hdr.aux_hi[3] * 1000.0f + 0.5f),
         (long)(hdr.aux_default[0] * 1000.0f + 0.5f),
         (long)(hdr.aux_default[1] * 1000.0f + 0.5f),
         (long)(hdr.aux_default[2] * 1000.0f + 0.5f),
         (long)(hdr.aux_default[3] * 1000.0f + 0.5f),
         dataset_version);
  return true;
}

static bool aux4_qspi_command_begin(void)
{
  if (QSPI_W25Qxx_BeginCommandMode() != QSPI_W25Qxx_OK) {
    return false;
  }
  if (QSPI_W25Qxx_ExitMemoryMapped() != QSPI_W25Qxx_OK) {
    QSPI_W25Qxx_EndCommandMode();
    return false;
  }
  return true;
}

static void aux4_qspi_command_end(void)
{
  if (QSPI_W25Qxx_EnterMemoryMapped() != QSPI_W25Qxx_OK) {
    printf("[AUX4] qspi remap failed after aux operation\r\n");
  }
  QSPI_W25Qxx_EndCommandMode();
}

static bool aux4_qspi_read_blob(uint32_t offset, aux4_bin_header_t *hdr_out)
{
  aux4_bin_header_t hdr;

  if (QSPI_W25Qxx_ReadBuffer_Slow(s_aux4_blob, offset, sizeof(hdr)) != QSPI_W25Qxx_OK) {
    return false;
  }
  memcpy(&hdr, s_aux4_blob, sizeof(hdr));
  if (!aux4_header_basic_valid(&hdr)) {
    return false;
  }
  if (QSPI_W25Qxx_ReadBuffer_Slow(s_aux4_blob, offset, hdr.total_bytes) != QSPI_W25Qxx_OK) {
    return false;
  }
  if (!aux4_blob_valid(s_aux4_blob, hdr.total_bytes, &hdr)) {
    return false;
  }
  if (hdr_out != NULL) {
    *hdr_out = hdr;
  }
  return true;
}

static bool aux4_qspi_find_best_slot(uint32_t *slot_offset, aux4_bin_header_t *slot_hdr)
{
  aux4_bin_header_t hdr0;
  aux4_bin_header_t hdr1;
  bool valid0;
  bool valid1;

  valid0 = aux4_qspi_read_blob(AUX4_SLOT0_OFFSET, &hdr0);
  valid1 = aux4_qspi_read_blob(AUX4_SLOT1_OFFSET, &hdr1);
  if (!valid0 && !valid1) {
    return false;
  }

  if (valid0 && (!valid1 || (hdr0.generation >= hdr1.generation))) {
    if (slot_offset != NULL) {
      *slot_offset = AUX4_SLOT0_OFFSET;
    }
    if (slot_hdr != NULL) {
      *slot_hdr = hdr0;
    }
    return aux4_qspi_read_blob(AUX4_SLOT0_OFFSET, NULL);
  }

  if (slot_offset != NULL) {
    *slot_offset = AUX4_SLOT1_OFFSET;
  }
  if (slot_hdr != NULL) {
    *slot_hdr = hdr1;
  }
  return aux4_qspi_read_blob(AUX4_SLOT1_OFFSET, NULL);
}

void DAC8568_Aux4_Init(void)
{
  s_loaded = 0u;
  s_source = AUX4_SOURCE_DEFAULT;
  s_parse_error_count = 0u;
  s_inject_count = 0u;
  s_last_item_index = 0u;
  s_generation = 0u;
  s_payload_checksum = 0u;
  aux4_restore_legacy_ranges();
  aux4_reset_schedules();
  aux4_set_default_active();
}

bool DAC8568_Aux4_LoadFromSd(const char *path)
{
  FIL file;
  FILINFO info;
  UINT bytes_read = 0u;
  FRESULT res;

  res = SD_Init();
  if (res != FR_OK) {
    s_parse_error_count++;
    printf("[AUX4] sd transient load failed: sd_init err=%d\r\n", (int)res);
    return false;
  }

  memset(&info, 0, sizeof(info));
  res = f_stat(path, &info);
  if (res != FR_OK) {
    printf("[AUX4] sd transient load missing: path=%s err=%d\r\n",
           (path != NULL) ? path : "(null)",
           (int)res);
    return false;
  }
  if ((info.fsize < AUX4_HEADER_BYTES) || (info.fsize > AUX4_SLOT_SIZE)) {
    s_parse_error_count++;
    printf("[AUX4] sd transient load bad size: path=%s size=%lu\r\n",
           (path != NULL) ? path : "(null)",
           (unsigned long)info.fsize);
    return false;
  }

  res = f_open(&file, path, FA_READ);
  if (res != FR_OK) {
    s_parse_error_count++;
    printf("[AUX4] sd transient load open failed: path=%s err=%d\r\n",
           (path != NULL) ? path : "(null)",
           (int)res);
    return false;
  }
  res = f_read(&file, s_aux4_blob, (UINT)info.fsize, &bytes_read);
  (void)f_close(&file);
  if ((res != FR_OK) || (bytes_read != (UINT)info.fsize)) {
    s_parse_error_count++;
    printf("[AUX4] sd transient load read failed: path=%s err=%d read=%u size=%lu\r\n",
           (path != NULL) ? path : "(null)",
           (int)res,
           (unsigned)bytes_read,
           (unsigned long)info.fsize);
    return false;
  }

  if (!aux4_load_blob_into_ram(s_aux4_blob, bytes_read, AUX4_SOURCE_SD)) {
    printf("[AUX4] sd transient load invalid: path=%s\r\n",
           (path != NULL) ? path : "(null)");
    return false;
  }
  return true;
}

bool DAC8568_Aux4_SyncFromSdToQspi(const char *path)
{
  aux4_bin_header_t sd_hdr;
  aux4_bin_header_t best_hdr;
  uint32_t best_offset = AUX4_SLOT0_OFFSET;
  uint32_t target_offset = AUX4_SLOT0_OFFSET;
  FIL file;
  FILINFO info;
  UINT bytes_read = 0u;
  FRESULT res;
  bool have_best;
  bool ok = false;

  res = SD_Init();
  if (res != FR_OK) {
    s_parse_error_count++;
    printf("[AUX4] sync skip: sd_init err=%d\r\n", (int)res);
    return false;
  }
  memset(&info, 0, sizeof(info));
  res = f_stat(path, &info);
  if (res != FR_OK) {
    printf("[AUX4] sync skip: missing path=%s err=%d\r\n",
           (path != NULL) ? path : "(null)",
           (int)res);
    return false;
  }
  if ((info.fsize < AUX4_HEADER_BYTES) || (info.fsize > AUX4_SLOT_SIZE)) {
    s_parse_error_count++;
    printf("[AUX4] sync reject: size=%lu max=%lu path=%s\r\n",
           (unsigned long)info.fsize,
           (unsigned long)AUX4_SLOT_SIZE,
           (path != NULL) ? path : "(null)");
    return false;
  }

  if (!aux4_qspi_command_begin()) {
    s_parse_error_count++;
    printf("[AUX4] sync failed: qspi busy\r\n");
    return false;
  }
  have_best = aux4_qspi_find_best_slot(&best_offset, &best_hdr);
  aux4_qspi_command_end();

  res = f_open(&file, path, FA_READ);
  if (res != FR_OK) {
    s_parse_error_count++;
    printf("[AUX4] sync open failed: path=%s err=%d\r\n",
           (path != NULL) ? path : "(null)",
           (int)res);
    return false;
  }
  res = f_read(&file, s_aux4_blob, (UINT)info.fsize, &bytes_read);
  (void)f_close(&file);
  if ((res != FR_OK) || (bytes_read != (UINT)info.fsize)) {
    s_parse_error_count++;
    printf("[AUX4] sync read failed: err=%d read=%u size=%lu path=%s\r\n",
           (int)res,
           (unsigned)bytes_read,
           (unsigned long)info.fsize,
           (path != NULL) ? path : "(null)");
    return false;
  }
  if (!aux4_blob_valid(s_aux4_blob, bytes_read, &sd_hdr)) {
    s_parse_error_count++;
    printf("[AUX4] sync reject: invalid a4b path=%s\r\n",
           (path != NULL) ? path : "(null)");
    return false;
  }

  if (have_best) {
    if ((sd_hdr.generation < best_hdr.generation) ||
        ((sd_hdr.generation == best_hdr.generation) &&
         (sd_hdr.payload_fnv1a32 != best_hdr.payload_fnv1a32))) {
      s_parse_error_count++;
      printf("[AUX4] sync reject: stale/conflicting generation sd=%lu best=%lu sd_crc=0x%08lX best_crc=0x%08lX\r\n",
             (unsigned long)sd_hdr.generation,
             (unsigned long)best_hdr.generation,
             (unsigned long)sd_hdr.payload_fnv1a32,
             (unsigned long)best_hdr.payload_fnv1a32);
      return false;
    }
    if ((sd_hdr.generation == best_hdr.generation) &&
        (sd_hdr.payload_fnv1a32 == best_hdr.payload_fnv1a32)) {
      printf("[AUX4] sync skip: qspi already has generation=%lu checksum=0x%08lX\r\n",
             (unsigned long)sd_hdr.generation,
             (unsigned long)sd_hdr.payload_fnv1a32);
      return DAC8568_Aux4_LoadFromQspi();
    }
    target_offset = (best_offset == AUX4_SLOT0_OFFSET) ? AUX4_SLOT1_OFFSET : AUX4_SLOT0_OFFSET;
  }

  if (!aux4_qspi_command_begin()) {
    s_parse_error_count++;
    printf("[AUX4] sync failed: qspi busy\r\n");
    return false;
  }

  if (QSPI_W25Qxx_BlockErase_64K(target_offset) != QSPI_W25Qxx_OK) {
    printf("[AUX4] sync erase failed: slot=0x%08lX\r\n", (unsigned long)target_offset);
    goto done;
  }
  if (QSPI_W25Qxx_WriteBuffer_Slow(s_aux4_blob, target_offset, sd_hdr.total_bytes) != QSPI_W25Qxx_OK) {
    printf("[AUX4] sync write failed: slot=0x%08lX bytes=%lu\r\n",
           (unsigned long)target_offset,
           (unsigned long)sd_hdr.total_bytes);
    goto done;
  }
  if (!aux4_qspi_read_blob(target_offset, NULL)) {
    printf("[AUX4] sync verify failed: slot=0x%08lX\r\n", (unsigned long)target_offset);
    goto done;
  }
  ok = aux4_load_blob_into_ram(s_aux4_blob, sd_hdr.total_bytes, AUX4_SOURCE_QSPI);
  printf("[AUX4] sync ok: path=%s slot=0x%08lX generation=%lu bytes=%lu checksum=0x%08lX\r\n",
         (path != NULL) ? path : "(null)",
         (unsigned long)target_offset,
         (unsigned long)sd_hdr.generation,
         (unsigned long)sd_hdr.total_bytes,
         (unsigned long)sd_hdr.payload_fnv1a32);

done:
  aux4_qspi_command_end();
  return ok;
}

bool DAC8568_Aux4_LoadFromQspi(void)
{
  aux4_bin_header_t hdr;
  uint32_t slot_offset = 0u;
  bool ok = false;

  if (!aux4_qspi_command_begin()) {
    s_parse_error_count++;
    printf("[AUX4] qspi load failed: qspi busy\r\n");
    return false;
  }

  if (aux4_qspi_find_best_slot(&slot_offset, &hdr)) {
    ok = aux4_load_blob_into_ram(s_aux4_blob, hdr.total_bytes, AUX4_SOURCE_QSPI);
    if (ok) {
      printf("[AUX4] qspi load ok: slot=0x%08lX generation=%lu bytes=%lu checksum=0x%08lX\r\n",
             (unsigned long)slot_offset,
             (unsigned long)hdr.generation,
             (unsigned long)hdr.total_bytes,
             (unsigned long)hdr.payload_fnv1a32);
    }
  } else {
    printf("[AUX4] qspi load missing valid slot, aux4_source=default\r\n");
  }

  aux4_qspi_command_end();
  if (!ok) {
    s_parse_error_count++;
    s_loaded = 0u;
    s_source = AUX4_SOURCE_DEFAULT;
    s_generation = 0u;
    s_payload_checksum = 0u;
    aux4_set_default_active();
  }
  return ok;
}

bool DAC8568_Aux4_VerifyWave(uint8_t partition_id,
                             uint32_t sample_rate_hz,
                             uint32_t sample_count,
                             uint32_t checksum)
{
  aux4_schedule_t *schedule;
  uint32_t expected_items;

  if (partition_id >= AUX4_FILE_COUNT) {
    return false;
  }
  schedule = &s_schedules[partition_id];
  if ((s_loaded == 0u) || (schedule->loaded == 0u)) {
    return false;
  }
  if ((sample_count == 0u) || (schedule->samples_per_item == 0u)) {
    return false;
  }
  expected_items = (sample_count + schedule->samples_per_item - 1u) / schedule->samples_per_item;

  if ((schedule->d8cw_sample_rate_hz != sample_rate_hz) ||
      (schedule->d8cw_sample_count != sample_count) ||
      (schedule->d8cw_checksum != checksum) ||
      (schedule->item_count != expected_items)) {
    schedule->loaded = 0u;
    schedule->verified = 0u;
    s_parse_error_count++;
    if (s_active_file_index == partition_id) {
      aux4_set_default_active();
    }
    printf("[AUX4] bind mismatch: file=%s aux_sps=%lu wave_sps=%lu aux_count=%lu wave_count=%lu aux_items=%lu expected_items=%lu aux_crc=0x%08lX wave_crc=0x%08lX\r\n",
           schedule->file_name,
           (unsigned long)schedule->d8cw_sample_rate_hz,
           (unsigned long)sample_rate_hz,
           (unsigned long)schedule->d8cw_sample_count,
           (unsigned long)sample_count,
           (unsigned long)schedule->item_count,
           (unsigned long)expected_items,
           (unsigned long)schedule->d8cw_checksum,
           (unsigned long)checksum);
    return false;
  }

  schedule->verified = 1u;
  printf("[AUX4] bind ok: file=%s items=%lu samples_per_item=%lu d8cw_count=%lu checksum=0x%08lX\r\n",
         schedule->file_name,
         (unsigned long)schedule->item_count,
         (unsigned long)schedule->samples_per_item,
         (unsigned long)sample_count,
         (unsigned long)checksum);
  return true;
}

void DAC8568_Aux4_SetActiveFile(const char *bin_path_or_name, bool reset_index)
{
  const char *name = aux4_basename(bin_path_or_name);
  int32_t idx = aux4_file_index(name);
  uint32_t primask = __get_PRIMASK();

  __disable_irq();
  if (!primask) {
    __DMB();
  }

  if (idx >= 0) {
    s_active_file_index = (uint8_t)idx;
    (void)snprintf(s_active_file_name, sizeof(s_active_file_name), "%s", s_aux4_files[idx]);
    if (reset_index) {
      s_schedules[idx].last_emitted_item = 0xFFFFFFFFu;
      s_pending_initial = 1u;
      s_default_samples_accum = 0u;
    }
    if ((s_loaded != 0u) &&
        (s_schedules[idx].loaded != 0u) &&
        (s_schedules[idx].verified != 0u) &&
        (s_schedules[idx].item_count > 0u)) {
      s_using_default = 0u;
      aux4_set_current(s_schedules[idx].values[0], 0u);
    } else {
      s_using_default = 1u;
      aux4_set_current(s_aux4_default_values, 0u);
    }
  } else {
    s_active_file_index = 0u;
    s_using_default = 1u;
    if (reset_index) {
      s_pending_initial = 1u;
      s_default_samples_accum = 0u;
    }
    (void)snprintf(s_active_file_name, sizeof(s_active_file_name), "%s", name);
    aux4_set_current(s_aux4_default_values, 0u);
  }

  __DMB();
  if (!primask) {
    __enable_irq();
  }
}

static bool aux4_default_due(uint32_t samples_elapsed)
{
  if (s_pending_initial != 0u) {
    s_pending_initial = 0u;
    return true;
  }
  s_default_samples_accum += samples_elapsed;
  if (s_default_samples_accum >= AUX4_DEFAULT_SAMPLES_PER_ITEM) {
    s_default_samples_accum %= AUX4_DEFAULT_SAMPLES_PER_ITEM;
    return true;
  }
  return false;
}

bool DAC8568_Aux4_ConsumeDue(uint8_t file_index,
                             uint32_t sample_index,
                             uint32_t samples_elapsed,
                             float values[DAC8568_AUX4_COUNT],
                             float volts[DAC8568_AUX4_COUNT])
{
  aux4_schedule_t *schedule;
  uint32_t end_sample;
  uint32_t item_index;

  if ((values == NULL) || (volts == NULL)) {
    return false;
  }
  if ((file_index >= AUX4_FILE_COUNT) ||
      (s_loaded == 0u) ||
      (s_schedules[file_index].loaded == 0u) ||
      (s_schedules[file_index].verified == 0u) ||
      (s_schedules[file_index].item_count == 0u)) {
    if (!aux4_default_due(samples_elapsed)) {
      return false;
    }
    s_using_default = 1u;
    aux4_set_current(s_aux4_default_values, 0u);
    aux4_copy4(values, s_current_values);
    aux4_copy4(volts, s_current_volts);
    s_inject_count++;
    return true;
  }

  schedule = &s_schedules[file_index];
  if (s_active_file_index != file_index) {
    s_active_file_index = file_index;
    (void)snprintf(s_active_file_name, sizeof(s_active_file_name), "%s", schedule->file_name);
    schedule->last_emitted_item = 0xFFFFFFFFu;
  }
  if (samples_elapsed == 0u) {
    samples_elapsed = 1u;
  }
  if (schedule->d8cw_sample_count == 0u) {
    return false;
  }
  if (sample_index >= schedule->d8cw_sample_count) {
    sample_index = 0u;
  }
  end_sample = sample_index + samples_elapsed - 1u;
  if ((end_sample < sample_index) || (end_sample >= schedule->d8cw_sample_count)) {
    item_index = 0u;
  } else {
    item_index = end_sample / schedule->samples_per_item;
    if (item_index >= schedule->item_count) {
      item_index = schedule->item_count - 1u;
    }
  }

  if ((s_pending_initial == 0u) && (item_index == schedule->last_emitted_item)) {
    return false;
  }

  s_pending_initial = 0u;
  s_using_default = 0u;
  schedule->last_emitted_item = item_index;
  aux4_set_current(schedule->values[item_index], item_index);
  aux4_copy4(values, s_current_values);
  aux4_copy4(volts, s_current_volts);
  s_inject_count++;
  return true;
}

void DAC8568_Aux4_GetStatus(DAC8568_Aux4Status_t *status)
{
  if (status == NULL) {
    return;
  }

  status->loaded = s_loaded;
  status->using_default = s_using_default;
  status->source = s_source;
  status->active_file_index = s_active_file_index;
  status->active_item_count = ((s_active_file_index < AUX4_FILE_COUNT) &&
                               (s_schedules[s_active_file_index].loaded != 0u) &&
                               (s_schedules[s_active_file_index].verified != 0u))
                                ? s_schedules[s_active_file_index].item_count
                                : 0u;
  status->inject_count = s_inject_count;
  status->parse_error_count = s_parse_error_count;
  status->last_item_index = s_last_item_index;
  status->generation = s_generation;
  status->payload_checksum = s_payload_checksum;
  (void)snprintf(status->active_file, sizeof(status->active_file), "%s", s_active_file_name);
  aux4_copy4(status->values, s_current_values);
  aux4_copy4(status->volts, s_current_volts);
  aux4_copy4(status->range_lo, s_aux4_lo);
  aux4_copy4(status->range_hi, s_aux4_hi);
  aux4_copy4(status->default_values, s_aux4_default_values);
}
