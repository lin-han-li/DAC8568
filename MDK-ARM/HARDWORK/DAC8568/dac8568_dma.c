#include "dac8568_dma.h"

#include "gpio.h"
#include "main.h"
#include "spi.h"
#include "tim.h"
#include "stm32h7xx_hal_dma_ex.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

#define DAC8568_CMD_WRITE_INPUT 0x00u
#define DAC8568_CMD_UPDATE_DAC 0x01u
#define DAC8568_CMD_WRITE_UPDATE_ALL 0x02u
#define DAC8568_CMD_WRITE_UPDATE_DAC 0x03u
#define DAC8568_CMD_SOFTWARE_RESET 0x07u
#define DAC8568_CMD_INTERNAL_REF 0x08u

#define DAC8568_CHANNEL_A 0x00u
#define DAC8568_CHANNEL_B 0x01u
#define DAC8568_CHANNEL_C 0x02u
#define DAC8568_CHANNEL_D 0x03u

/*
 * DAC8568IDPW(0~5V) + 运放移位放大：
 * Vout = 4 * (Vi - 2.5V)
 * 这里按“对外输出 ±5V”生成波形。
 */
#define DAC8568_MAX_VOLTAGE 5.0f
#define DAC8568_MIN_VOLTAGE -5.0f
#define DAC8568_VREF_MV 2500.0f

#define LUT_BITS 10u
#define LUT_SIZE (1u << LUT_BITS)
#define LUT_PHASE_SHIFT (32u - LUT_BITS)

#define WAVE_FREQ_A_HZ 12000.0
#define WAVE_FREQ_B_HZ 6000.0
#define WAVE_FREQ_C_HZ 3000.0
#define WAVE_FREQ_D_HZ 1000.0

#define DAC8568_WORDS_PER_SAMPLE 4u
#define DAC8568_SAMPLES_PER_HALF 8191u
#define DAC8568_TX_BUF_WORDS (DAC8568_SAMPLES_PER_HALF * DAC8568_WORDS_PER_SAMPLE * 2u)
#define DAC8568_TX_HALF_WORDS (DAC8568_TX_BUF_WORDS / 2u)

#if (DAC8568_TX_BUF_WORDS > 65535u)
#error "DAC8568_TX_BUF_WORDS exceeds HAL_SPI_Transmit_DMA(uint16_t Size) limit; reduce DAC8568_SAMPLES_PER_HALF."
#endif

/*
 * DAC8568 (TI) 32-bit frame (MSB-first):
 *   [ 4'b0000 | CMD(4) | ADDR(4) | DATA(16) | 4'b0000 ]
 * This is consistent with the original project implementation under
 * `DAC8568_H750_Hardware_SPI_10V/.../User/dac8568`.
 */
#define DAC8568_FRAME_PREFIX(cmd, channel) (((uint32_t)(cmd) << 24) | ((uint32_t)(channel) << 20))
#define DAC8568_FRAME_A_PREFIX DAC8568_FRAME_PREFIX(DAC8568_CMD_WRITE_INPUT, DAC8568_CHANNEL_A)
#define DAC8568_FRAME_B_PREFIX DAC8568_FRAME_PREFIX(DAC8568_CMD_WRITE_INPUT, DAC8568_CHANNEL_B)
#define DAC8568_FRAME_C_PREFIX DAC8568_FRAME_PREFIX(DAC8568_CMD_WRITE_INPUT, DAC8568_CHANNEL_C)
#define DAC8568_FRAME_D_PREFIX DAC8568_FRAME_PREFIX(DAC8568_CMD_WRITE_UPDATE_ALL, DAC8568_CHANNEL_D)
#define DAC8568_INTERNAL_REF_ENABLE_FRAME ((((uint32_t)DAC8568_CMD_INTERNAL_REF) << 24) | 0x01u)
#define DAC8568_SOFT_RESET_FRAME (((uint32_t)DAC8568_CMD_SOFTWARE_RESET) << 24)

#define DAC8568_REF_REFRESH_INTERVAL_MS 250u
#define DAC8568_STAGNANT_WINDOW_MS 40u
#define DAC8568_STAGNANT_LIMIT 3u
#define DAC8568_QSPI_MMAP_BASE 0x90000000u
#define DAC8568_QSPI_MMAP_LIMIT 0x92000000u

#define DAC8568_RECOVER_REASON_NONE 0u
#define DAC8568_RECOVER_REASON_SPI_ERROR 1u
#define DAC8568_RECOVER_REASON_STAGNANT 2u

static uint32_t g_sample_rate_hz = 120000u;

static uint32_t g_phase_a = 0u;
static uint32_t g_phase_b = 0u;
static uint32_t g_phase_c = 0u;
static uint32_t g_phase_d = 0u;

static uint32_t g_phase_inc_a = 0u;
static uint32_t g_phase_inc_b = 0u;
static uint32_t g_phase_inc_c = 0u;
static uint32_t g_phase_inc_d = 0u;

static uint16_t g_lut_sine[LUT_SIZE];
static uint16_t g_lut_triangle[LUT_SIZE];
static uint16_t g_lut_saw[LUT_SIZE];
static uint16_t g_lut_square[LUT_SIZE];

__attribute__((section(".ram_d2"), aligned(32))) static uint32_t g_tx_buf[DAC8568_TX_BUF_WORDS];

static const uint32_t *g_wave_frames = NULL;
static uint32_t g_wave_words_total = 0u;
static volatile uint32_t g_wave_word_index = 0u;
static const uint16_t *g_wave_codes16 = NULL;
static uint32_t g_wave_code_samples = 0u;
static volatile uint32_t g_wave_code_index = 0u;

static volatile uint32_t g_tx_ok = 0u;
static volatile uint32_t g_tx_fail = 0u;
static volatile uint32_t g_tick_skip = 0u;
static volatile uint32_t g_tick_count = 0u;
static volatile uint32_t g_sample_count = 0u;
static volatile uint8_t g_stream_running = 0u;
static volatile uint32_t g_dma_buf_cycles = 0u;
static volatile uint8_t g_ref_refresh_pending = 0u;
static volatile uint32_t g_recover_count = 0u;
static volatile uint32_t g_recover_reason = DAC8568_RECOVER_REASON_NONE;
static volatile uint32_t g_ref_rearm_count = 0u;
static volatile uint32_t g_ref_refresh_count = 0u;
static volatile uint32_t g_stagnant_count = 0u;

static uint32_t g_service_last_tick = 0u;
static uint32_t g_service_last_samples = 0u;
static uint32_t g_service_last_fail = 0u;
static uint32_t g_last_ref_refresh_tick = 0u;
static volatile DAC8568_SourceMode_t g_source_mode = DAC8568_SOURCE_LUT;
static const uint16_t *g_qspi_wave_data = NULL;
static uint32_t g_qspi_wave_samples = 0u;
static uint32_t g_qspi_wave_index = 0u;

static uint32_t dac8568_get_tx_sample_counter(void) {
  /* Derived from DMA progress for sub-buffer resolution (avoids 8191-sample quantization). */
  if (g_stream_running == 0u) {
    return g_sample_count;
  }

  const uint32_t samples_per_buf = DAC8568_SAMPLES_PER_HALF * 2u;

  uint32_t cycles_a = g_dma_buf_cycles;
  uint32_t remaining_words = __HAL_DMA_GET_COUNTER(&hdma_spi1_tx);
  uint32_t cycles_b = g_dma_buf_cycles;
  if (cycles_b != cycles_a) {
    cycles_a = cycles_b;
    remaining_words = __HAL_DMA_GET_COUNTER(&hdma_spi1_tx);
  }

  if (remaining_words > DAC8568_TX_BUF_WORDS) {
    remaining_words = DAC8568_TX_BUF_WORDS;
  }

  uint32_t words_done = DAC8568_TX_BUF_WORDS - remaining_words;
  uint32_t samples_in_buf = words_done / DAC8568_WORDS_PER_SAMPLE;
  if (samples_in_buf > samples_per_buf) {
    samples_in_buf = samples_per_buf;
  }

  return cycles_a * samples_per_buf + samples_in_buf;
}

static void dac8568_tim12_stop(void) {
  (void)HAL_TIM_Base_Stop(&htim12);
}

static uint32_t dac8568_tim12_get_timclk_hz(void) {
  uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();

  /*
   * On STM32H7, if APB prescaler != 1, the timer clock is doubled.
   * TIM12 is on APB1 (D2 domain).
   */
  if ((RCC->D2CFGR & RCC_D2CFGR_D2PPRE1) != RCC_D2CFGR_D2PPRE1_DIV1) {
    return pclk1 * 2u;
  }
  return pclk1;
}

static void dac8568_tim12_apply_sample_rate(uint32_t sample_rate_hz) {
  if (sample_rate_hz == 0u) {
    sample_rate_hz = 1u;
  }

  uint32_t timclk_hz = dac8568_tim12_get_timclk_hz();
  if (timclk_hz == 0u) {
    timclk_hz = 1u;
  }

  /*
   * Fit TIM12 (16-bit) by selecting prescaler/ARR.
   * target_hz = timclk / ((PSC+1) * (ARR+1))
   */
  uint32_t ticks = timclk_hz / sample_rate_hz;
  if (ticks == 0u) {
    ticks = 1u;
  }

  uint32_t prescaler = 0u;
  if (ticks > 65536u) {
    prescaler = (ticks + 65535u) / 65536u - 1u;
    if (prescaler > 0xFFFFu) {
      prescaler = 0xFFFFu;
    }
  }

  uint32_t arr_plus1 = timclk_hz / (sample_rate_hz * (prescaler + 1u));
  if (arr_plus1 == 0u) {
    arr_plus1 = 1u;
  }
  if (arr_plus1 > 65536u) {
    arr_plus1 = 65536u;
  }

  __HAL_TIM_SET_PRESCALER(&htim12, prescaler);
  __HAL_TIM_SET_AUTORELOAD(&htim12, arr_plus1 - 1u);
}

static HAL_StatusTypeDef dac8568_tim12_start(void) {
  /*
   * TIM12 is configured by CubeMX with TRGO = Update Event.
   * We override PSC/ARR at runtime to match g_sample_rate_hz, so the DAC sample
   * rate is decoupled from the generated tim.c defaults.
   */
  MX_TIM12_Init();
  dac8568_tim12_apply_sample_rate(g_sample_rate_hz);
  __HAL_TIM_SET_COUNTER(&htim12, 0u);
  __HAL_TIM_CLEAR_FLAG(&htim12, TIM_FLAG_UPDATE);
  if (HAL_TIM_Base_Start(&htim12) != HAL_OK) {
    return HAL_ERROR;
  }
  SET_BIT(htim12.Instance->EGR, TIM_EGR_UG);
  return HAL_OK;
}

static uint16_t dac8568_voltage_to_code(float voltage) {
  float clamped = voltage;
  if (clamped > DAC8568_MAX_VOLTAGE) {
    clamped = DAC8568_MAX_VOLTAGE;
  } else if (clamped < DAC8568_MIN_VOLTAGE) {
    clamped = DAC8568_MIN_VOLTAGE;
  }

  /* Vi = Vout/4 + 2.5V（运放前 DAC 输出电压） */
  float real_voltage = clamped / 4.0f + 2.5f;
  float scaled = (real_voltage / 2.0f) * 1000.0f / DAC8568_VREF_MV;
  int32_t code = (int32_t)(scaled * 65535.0f + 0.5f);
  if (code < 0) {
    code = 0;
  } else if (code > 65535) {
    code = 65535;
  }
  return (uint16_t)code;
}

__STATIC_FORCEINLINE uint32_t dac8568_build_frame32(uint8_t cmd, uint8_t channel, uint16_t code) {
  return ((uint32_t)(cmd & 0x0Fu) << 24) | ((uint32_t)(channel & 0x0Fu) << 20) | ((uint32_t)code << 4);
}

static HAL_StatusTypeDef dac8568_spi_tx_word32_blocking(uint32_t word) {
  return HAL_SPI_Transmit(&hspi1, (uint8_t *)&word, 1u, 1000u);
}

static HAL_StatusTypeDef dac8568_spi_tx_word32_retry(uint32_t word, uint32_t retries) {
  for (uint32_t attempt = 0u; attempt <= retries; attempt++) {
    HAL_StatusTypeDef st = dac8568_spi_tx_word32_blocking(word);
    if (st == HAL_OK) {
      return HAL_OK;
    }
    (void)HAL_SPI_Abort(&hspi1);
    DAC8568_SPI1_ReInit(); /* Re-apply CubeMX-generated config. */
    HAL_Delay(2);
  }
  return HAL_ERROR;
}

static HAL_StatusTypeDef dac8568_rearm_internal_ref(uint32_t retries) {
  HAL_StatusTypeDef st = dac8568_spi_tx_word32_retry(DAC8568_INTERNAL_REF_ENABLE_FRAME, retries);
  if (st == HAL_OK) {
    g_ref_rearm_count++;
  }
  return st;
}

static HAL_StatusTypeDef dac8568_soft_reset_and_rearm(void) {
  HAL_StatusTypeDef st_reset = dac8568_spi_tx_word32_retry(DAC8568_SOFT_RESET_FRAME, 1u);
  HAL_Delay(2);
  HAL_StatusTypeDef st_ref = dac8568_rearm_internal_ref(2u);
  HAL_Delay(10);
  return ((st_reset == HAL_OK) && (st_ref == HAL_OK)) ? HAL_OK : HAL_ERROR;
}

static void dac8568_prepare_lut(void) {
  for (uint32_t i = 0u; i < LUT_SIZE; ++i) {
    float phase = (2.0f * 3.1415926535f * (float)i) / (float)LUT_SIZE;

    /* Sine */
    float v_sine = DAC8568_MAX_VOLTAGE * sinf(phase);
    g_lut_sine[i] = dac8568_voltage_to_code(v_sine);

    /* Triangle: -1..+1 */
    float x = (float)i / (float)LUT_SIZE; /* 0..1 */
    float tri = (x < 0.5f) ? (4.0f * x - 1.0f) : (3.0f - 4.0f * x);
    g_lut_triangle[i] = dac8568_voltage_to_code(DAC8568_MAX_VOLTAGE * tri);

    /* Saw: -1..+1 */
    float saw = 2.0f * x - 1.0f;
    g_lut_saw[i] = dac8568_voltage_to_code(DAC8568_MAX_VOLTAGE * saw);

    /* Square: -1 / +1 */
    float sq = (x < 0.5f) ? 1.0f : -1.0f;
    g_lut_square[i] = dac8568_voltage_to_code(DAC8568_MAX_VOLTAGE * sq);
  }
}

static void dac8568_update_phase_inc(uint32_t sample_rate_hz) {
  g_phase_inc_a = (uint32_t)((WAVE_FREQ_A_HZ * 4294967296.0) / (double)sample_rate_hz);
  g_phase_inc_b = (uint32_t)((WAVE_FREQ_B_HZ * 4294967296.0) / (double)sample_rate_hz);
  g_phase_inc_c = (uint32_t)((WAVE_FREQ_C_HZ * 4294967296.0) / (double)sample_rate_hz);
  g_phase_inc_d = (uint32_t)((WAVE_FREQ_D_HZ * 4294967296.0) / (double)sample_rate_hz);
}

static void dac8568_dcache_clean(void *addr, size_t bytes) {
  uintptr_t start = (uintptr_t)addr & ~(uintptr_t)31u;
  uintptr_t end = ((uintptr_t)addr + bytes + 31u) & ~(uintptr_t)31u;
  SCB_CleanDCache_by_Addr((uint32_t *)start, (int32_t)(end - start));
}

static void dac8568_fill_samples(uint32_t *dst, uint32_t sample_count) {
  uint32_t *dst_base = dst;

  if ((g_wave_codes16 != NULL) && (g_wave_code_samples != 0u)) {
    uint32_t idx = g_wave_code_index;

    for (uint32_t i = 0u; i < sample_count; i++) {
      const uint16_t *sample = &g_wave_codes16[idx * DAC8568_WORDS_PER_SAMPLE];
      *dst++ = DAC8568_FRAME_A_PREFIX | ((uint32_t)sample[0] << 4);
      *dst++ = DAC8568_FRAME_B_PREFIX | ((uint32_t)sample[1] << 4);
      *dst++ = DAC8568_FRAME_C_PREFIX | ((uint32_t)sample[2] << 4);
      *dst++ = DAC8568_FRAME_D_PREFIX | ((uint32_t)sample[3] << 4);
      idx++;
      if (idx >= g_wave_code_samples) {
        idx = 0u;
      }
    }

    g_wave_code_index = idx;

    g_tick_count += sample_count;
    g_sample_count += sample_count;
    g_tx_ok += sample_count;

    if ((sample_count > 0u) && (g_ref_refresh_pending != 0u)) {
      g_ref_refresh_pending = 0u;
      dst_base[0] = DAC8568_INTERNAL_REF_ENABLE_FRAME;
      g_ref_refresh_count++;
    }
    return;
  }

  if ((g_wave_frames != NULL) && (g_wave_words_total != 0u)) {
    uint32_t words_to_copy = sample_count * DAC8568_WORDS_PER_SAMPLE;
    uint32_t idx = g_wave_word_index;

    while (words_to_copy > 0u) {
      uint32_t chunk = g_wave_words_total - idx;
      if (chunk > words_to_copy) {
        chunk = words_to_copy;
      }
      memcpy(dst, &g_wave_frames[idx], (size_t)chunk * sizeof(uint32_t));
      dst += chunk;
      words_to_copy -= chunk;
      idx += chunk;
      if (idx >= g_wave_words_total) {
        idx = 0u;
      }
    }

    g_wave_word_index = idx;

    g_tick_count += sample_count;
    g_sample_count += sample_count;
    g_tx_ok += sample_count;

    if ((sample_count > 0u) && (g_ref_refresh_pending != 0u)) {
      g_ref_refresh_pending = 0u;
      dst_base[0] = DAC8568_INTERNAL_REF_ENABLE_FRAME;
      g_ref_refresh_count++;
    }
    return;
  }

  uint32_t phase_a = g_phase_a;
  uint32_t phase_b = g_phase_b;
  uint32_t phase_c = g_phase_c;
  uint32_t phase_d = g_phase_d;
  uint32_t qspi_index = g_qspi_wave_index;

  const uint32_t inc_a = g_phase_inc_a;
  const uint32_t inc_b = g_phase_inc_b;
  const uint32_t inc_c = g_phase_inc_c;
  const uint32_t inc_d = g_phase_inc_d;
  const uint8_t use_qspi = (g_source_mode == DAC8568_SOURCE_QSPI) &&
                           (g_qspi_wave_data != NULL) &&
                           (g_qspi_wave_samples > 0u);

  for (uint32_t i = 0u; i < sample_count; i++) {
    uint16_t code_a;
    uint16_t code_b;
    uint16_t code_c;
    uint16_t code_d;

    if (use_qspi != 0u) {
      const uint16_t *sample = &g_qspi_wave_data[qspi_index * DAC8568_WORDS_PER_SAMPLE];
      code_a = sample[0];
      code_b = sample[1];
      code_c = sample[2];
      code_d = sample[3];
      qspi_index++;
      if (qspi_index >= g_qspi_wave_samples) {
        qspi_index = 0u;
      }
    } else {
      code_a = g_lut_sine[phase_a >> LUT_PHASE_SHIFT];
      phase_a += inc_a;
      code_b = g_lut_triangle[phase_b >> LUT_PHASE_SHIFT];
      phase_b += inc_b;
      code_c = g_lut_saw[phase_c >> LUT_PHASE_SHIFT];
      phase_c += inc_c;
      code_d = g_lut_square[phase_d >> LUT_PHASE_SHIFT];
      phase_d += inc_d;
    }

    *dst++ = DAC8568_FRAME_A_PREFIX | ((uint32_t)code_a << 4);
    *dst++ = DAC8568_FRAME_B_PREFIX | ((uint32_t)code_b << 4);
    *dst++ = DAC8568_FRAME_C_PREFIX | ((uint32_t)code_c << 4);
    *dst++ = DAC8568_FRAME_D_PREFIX | ((uint32_t)code_d << 4);
  }

  if (use_qspi != 0u) {
    g_qspi_wave_index = qspi_index;
  } else {
    g_phase_a = phase_a;
    g_phase_b = phase_b;
    g_phase_c = phase_c;
    g_phase_d = phase_d;
  }

  g_tick_count += sample_count;
  g_sample_count += sample_count;
  g_tx_ok += sample_count;

  if ((sample_count > 0u) && (g_ref_refresh_pending != 0u)) {
    g_ref_refresh_pending = 0u;
    dst_base[0] = DAC8568_INTERNAL_REF_ENABLE_FRAME;
    g_ref_refresh_count++;
  }
}

void DAC8568_DMA_SetWaveFrames(const uint32_t *mm_frames, uint32_t sample_count) {
  if (!mm_frames || sample_count == 0u) {
    DAC8568_DMA_DisableWaveFrames();
    return;
  }
  g_wave_codes16 = NULL;
  g_wave_code_samples = 0u;
  g_wave_code_index = 0u;
  g_wave_frames = mm_frames;
  g_wave_words_total = sample_count * DAC8568_WORDS_PER_SAMPLE;
  g_wave_word_index = 0u;
}

void DAC8568_DMA_SetWaveCodes16(const uint16_t *mm_codes, uint32_t sample_count) {
  if (!mm_codes || sample_count == 0u) {
    DAC8568_DMA_DisableWaveFrames();
    return;
  }
  g_wave_frames = NULL;
  g_wave_words_total = 0u;
  g_wave_word_index = 0u;
  g_wave_codes16 = mm_codes;
  g_wave_code_samples = sample_count;
  g_wave_code_index = 0u;
}

void DAC8568_DMA_DisableWaveFrames(void) {
  g_wave_frames = NULL;
  g_wave_words_total = 0u;
  g_wave_word_index = 0u;
  g_wave_codes16 = NULL;
  g_wave_code_samples = 0u;
  g_wave_code_index = 0u;
}

static void dac8568_dma_on_half(void) {
  dac8568_fill_samples(&g_tx_buf[0], DAC8568_SAMPLES_PER_HALF);
  dac8568_dcache_clean(&g_tx_buf[0], (size_t)DAC8568_TX_HALF_WORDS * sizeof(uint32_t));
}

static void dac8568_dma_on_full(void) {
  dac8568_fill_samples(&g_tx_buf[DAC8568_TX_HALF_WORDS], DAC8568_SAMPLES_PER_HALF);
  dac8568_dcache_clean(&g_tx_buf[DAC8568_TX_HALF_WORDS], (size_t)DAC8568_TX_HALF_WORDS * sizeof(uint32_t));
}

void DAC8568_DMA_Init(uint32_t sample_rate_hz) {
  g_sample_rate_hz = (sample_rate_hz == 0u) ? 48000u : sample_rate_hz;
  g_phase_a = g_phase_b = g_phase_c = g_phase_d = 0u;
  g_qspi_wave_index = 0u;
  dac8568_update_phase_inc(g_sample_rate_hz);

  /*
   * Power-up pins:
   * - CLR released (high)
   * - LDAC low enables immediate output updates by SPI commands
   *   (LDAC high would defer updates until an LDAC pulse).
   */
  HAL_GPIO_WritePin(DAC8568_CLR_GPIO_Port, DAC8568_CLR_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(DAC8568_LDAC_GPIO_Port, DAC8568_LDAC_Pin, GPIO_PIN_RESET);
  HAL_Delay(20);

  dac8568_prepare_lut();

  /* Startup robustness: reset + internal ref enable (retry). */
  (void)dac8568_soft_reset_and_rearm();
  HAL_Delay(100);
}

void DAC8568_DMA_Start(void) {
  g_stream_running = 0u;
  dac8568_tim12_stop();

  (void)HAL_SPI_Abort(&hspi1);

  /* Ensure LDAC allows continuous updates during streaming. */
  HAL_GPIO_WritePin(DAC8568_LDAC_GPIO_Port, DAC8568_LDAC_Pin, GPIO_PIN_RESET);

  g_tx_ok = 0u;
  g_tx_fail = 0u;
  g_tick_skip = 0u;
  g_tick_count = 0u;
  g_sample_count = 0u;
  g_qspi_wave_index = 0u;

  /* Prefill both halves before starting the circular DMA stream. */
  dac8568_fill_samples(&g_tx_buf[0], DAC8568_SAMPLES_PER_HALF);
  dac8568_fill_samples(&g_tx_buf[DAC8568_TX_HALF_WORDS], DAC8568_SAMPLES_PER_HALF);
  dac8568_dcache_clean(g_tx_buf, sizeof(g_tx_buf));

  /* Reset stats after prefill (phase keeps advancing correctly). */
  g_tx_ok = 0u;
  g_tick_count = 0u;
  g_sample_count = 0u;
  g_dma_buf_cycles = 0u;
  g_stagnant_count = 0u;

  /*
   * TIM-paced streaming without 240 kHz IRQ:
   *
   * Use SPI1 TX DMA request (backpressure by TX FIFO space), and gate it by DMAMUX
   * synchronization on TIM12_TRGO. Each timer event authorizes 4 DMA requests
   * (A/B/C/D words), achieving the target sample rate while avoiding CPU load.
   *
   * Note: DMAMUX sync signal supports TIM12_TRGO on STM32H7; TIM2_TRGO is not
   * available as a DMAMUX sync source in this HAL/MCU.
   */
  HAL_DMA_MuxSyncConfigTypeDef sync = {0};
  sync.SyncSignalID = HAL_DMAMUX1_SYNC_TIM12_TRGO;
  sync.SyncPolarity = HAL_DMAMUX_SYNC_RISING;
  sync.SyncEnable = ENABLE;
  sync.EventEnable = DISABLE;
  sync.RequestNumber = DAC8568_WORDS_PER_SAMPLE;
  if (HAL_DMAEx_ConfigMuxSync(&hdma_spi1_tx, &sync) != HAL_OK) {
    g_tx_fail++;
    return;
  }

  if (HAL_SPI_Transmit_DMA(&hspi1, (uint8_t *)g_tx_buf, (uint16_t)DAC8568_TX_BUF_WORDS) != HAL_OK) {
    g_tx_fail++;
    return;
  }

  if (dac8568_tim12_start() != HAL_OK) {
    g_tx_fail++;
    return;
  }

  g_stream_running = 1u;
  g_ref_refresh_pending = 1u;
  g_service_last_samples = dac8568_get_tx_sample_counter();
  g_service_last_fail = g_tx_fail;
  g_service_last_tick = HAL_GetTick();
  g_last_ref_refresh_tick = g_service_last_tick;
}

void DAC8568_DMA_OnTimerTick(void) {
  /* Not used in SPI+DMA streaming mode. */
  (void)0;
}

void DAC8568_DMA_OnTxComplete(void) {
  /* Stream is driven by circular DMA; filling is done in HAL SPI callbacks. */
  (void)0;
}

void DAC8568_DMA_GetStats(uint32_t *tx_ok, uint32_t *tx_fail, uint32_t *tick_skip) {
  if (tx_ok != NULL) {
    *tx_ok = dac8568_get_tx_sample_counter();
  }
  if (tx_fail != NULL) {
    *tx_fail = g_tx_fail;
  }
  if (tick_skip != NULL) {
    *tick_skip = g_tick_skip;
  }
}

void DAC8568_DMA_UpdateSampleRate(uint32_t sample_rate_hz) {
  if (sample_rate_hz == 0u) {
    return;
  }
  g_sample_rate_hz = sample_rate_hz;
  dac8568_update_phase_inc(g_sample_rate_hz);
}

void DAC8568_DMA_GetTickCounter(uint32_t *tick_count) {
  if (tick_count != NULL) {
    *tick_count = g_tick_count;
  }
}

void DAC8568_DMA_GetSampleCounter(uint32_t *sample_count) {
  if (sample_count != NULL) {
    *sample_count = dac8568_get_tx_sample_counter();
  }
}

void DAC8568_DMA_Service(void) {
  if (g_stream_running == 0u) {
    return;
  }

  uint32_t now = HAL_GetTick();
  uint32_t samples = dac8568_get_tx_sample_counter();
  uint32_t fails = g_tx_fail;
  uint32_t recover_reason = DAC8568_RECOVER_REASON_NONE;

  if (fails != g_service_last_fail) {
    g_service_last_fail = fails;
    recover_reason |= DAC8568_RECOVER_REASON_SPI_ERROR;
  }

  if (samples == g_service_last_samples) {
    if ((now - g_service_last_tick) >= DAC8568_STAGNANT_WINDOW_MS) {
      g_service_last_tick = now;
      g_stagnant_count++;
      if (g_stagnant_count >= DAC8568_STAGNANT_LIMIT) {
        recover_reason |= DAC8568_RECOVER_REASON_STAGNANT;
      }
    }
  } else {
    g_service_last_samples = samples;
    g_service_last_tick = now;
    g_stagnant_count = 0u;
  }

  if ((now - g_last_ref_refresh_tick) >= DAC8568_REF_REFRESH_INTERVAL_MS) {
    g_ref_refresh_pending = 1u;
    g_last_ref_refresh_tick = now;
  }

  if (recover_reason == DAC8568_RECOVER_REASON_NONE) {
    return;
  }

  g_recover_reason = recover_reason;
  g_recover_count++;

  g_stream_running = 0u;
  dac8568_tim12_stop();
  (void)HAL_SPI_Abort(&hspi1);

  (void)dac8568_soft_reset_and_rearm();
  DAC8568_DMA_Start();
}

void DAC8568_DMA_GetHealth(uint32_t *recover_count, uint32_t *recover_reason,
                           uint32_t *ref_rearm_count, uint32_t *ref_refresh_count,
                           uint32_t *stagnant_count) {
  if (recover_count != NULL) {
    *recover_count = g_recover_count;
  }
  if (recover_reason != NULL) {
    *recover_reason = g_recover_reason;
  }
  if (ref_rearm_count != NULL) {
    *ref_rearm_count = g_ref_rearm_count;
  }
  if (ref_refresh_count != NULL) {
    *ref_refresh_count = g_ref_refresh_count;
  }
  if (stagnant_count != NULL) {
    *stagnant_count = g_stagnant_count;
  }
}

int32_t DAC8568_DMA_UseQspiWave(uint32_t qspi_mmap_addr, uint32_t sample_count,
                                uint32_t sample_rate_hz) {
  uint8_t restart_stream = 0u;

  if ((qspi_mmap_addr < DAC8568_QSPI_MMAP_BASE) || (qspi_mmap_addr >= DAC8568_QSPI_MMAP_LIMIT)) {
    return -1;
  }
  if (sample_count == 0u) {
    return -2;
  }

  if (g_stream_running != 0u) {
    restart_stream = 1u;
    g_stream_running = 0u;
    dac8568_tim12_stop();
    (void)HAL_SPI_Abort(&hspi1);
  }

  g_qspi_wave_data = (const uint16_t *)(uintptr_t)qspi_mmap_addr;
  g_qspi_wave_samples = sample_count;
  g_qspi_wave_index = 0u;
  g_source_mode = DAC8568_SOURCE_QSPI;

  if (sample_rate_hz != 0u) {
    g_sample_rate_hz = sample_rate_hz;
    dac8568_update_phase_inc(g_sample_rate_hz);
  }

  if (restart_stream != 0u) {
    DAC8568_DMA_Start();
  }
  return 0;
}

void DAC8568_DMA_UseBuiltInWave(void) {
  uint8_t restart_stream = 0u;

  if (g_stream_running != 0u) {
    restart_stream = 1u;
    g_stream_running = 0u;
    dac8568_tim12_stop();
    (void)HAL_SPI_Abort(&hspi1);
  }

  g_source_mode = DAC8568_SOURCE_LUT;
  g_qspi_wave_data = NULL;
  g_qspi_wave_samples = 0u;
  g_qspi_wave_index = 0u;

  if (restart_stream != 0u) {
    DAC8568_DMA_Start();
  }
}

DAC8568_SourceMode_t DAC8568_DMA_GetSourceMode(void) {
  return g_source_mode;
}

void DAC8568_OutputFixedVoltage(float voltage) {
  if (g_stream_running != 0u) {
    return;
  }

  uint16_t code = dac8568_voltage_to_code(voltage);
  uint32_t a = dac8568_build_frame32(DAC8568_CMD_WRITE_INPUT, DAC8568_CHANNEL_A, code);
  uint32_t b = dac8568_build_frame32(DAC8568_CMD_WRITE_INPUT, DAC8568_CHANNEL_B, code);
  uint32_t c = dac8568_build_frame32(DAC8568_CMD_WRITE_INPUT, DAC8568_CHANNEL_C, code);
  uint32_t d = dac8568_build_frame32(DAC8568_CMD_WRITE_UPDATE_ALL, DAC8568_CHANNEL_D, code);

  (void)dac8568_spi_tx_word32_blocking(a);
  (void)dac8568_spi_tx_word32_blocking(b);
  (void)dac8568_spi_tx_word32_blocking(c);
  (void)dac8568_spi_tx_word32_blocking(d);

  /*
   * Some boards wire LDAC and expect a pulse to latch, even if UPDATE commands
   * are used. Pulsing here is harmless for the streaming mode (which never
   * calls this function) and helps diagnostics in fixed-voltage mode.
   */
  HAL_GPIO_WritePin(DAC8568_LDAC_GPIO_Port, DAC8568_LDAC_Pin, GPIO_PIN_RESET);
  for (volatile uint32_t i = 0u; i < 400u; i++) {
    __NOP();
  }
  HAL_GPIO_WritePin(DAC8568_LDAC_GPIO_Port, DAC8568_LDAC_Pin, GPIO_PIN_SET);
}

void HAL_SPI_TxHalfCpltCallback(SPI_HandleTypeDef *hspi) {
  if (hspi == &hspi1) {
    if (g_stream_running != 0u) {
      dac8568_dma_on_half();
    }
  }
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
  if (hspi == &hspi1) {
    if (g_stream_running != 0u) {
      g_dma_buf_cycles++;
      dac8568_dma_on_full();
    }
  }
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi) {
  if (hspi == &hspi1) {
    g_tx_fail++;
  }
}
