/**
 * @file lv_port_indev.c
 */

/* Copy this file as "lv_port_indev.c" and set this value to "1" to enable content */
#if 1

/*********************
 * INCLUDES
 *********************/
#include "lv_port_indev.h"
#include "main.h"
#include "tim.h"
#include "stm32h7xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

/*********************
 * DEFINES
 *********************/
#define KEY_EVENT_DEBOUNCE_MS 30U
#define KEY_COUNT 4U
#define ENCODER_STEP_TICKS 2U
#define ENCODER_PENDING_LIMIT 32
#define ENCODER_DIR_INVERT 0U
#define BEEP_ON_TIME_MS 18U

/**********************
 * TYPEDEFS
 **********************/
typedef struct
{
    GPIO_TypeDef *gpio_port;
    uint16_t gpio_pin;
    uint32_t lv_key;
} key_map_t;

/**********************
 * STATIC PROTOTYPES
 **********************/
static void keypad_read(lv_indev_t *indev, lv_indev_data_t *data);
static void pop_ui_event(uint32_t *key_out, bool *valid_out);
static void key_latch_refresh(void);
static void encoder_poll_and_queue(void);
static void buzzer_trigger(void);
static void buzzer_service(void);

/**********************
 * STATIC VARIABLES
 **********************/
static lv_indev_t *indev_keypad = NULL;
static lv_group_t *indev_group = NULL;

static volatile uint32_t s_key_pending_mask = 0U;
static volatile uint32_t s_key_latched_mask = 0U;
static volatile uint32_t s_key_last_tick[KEY_COUNT] = {0U, 0U, 0U, 0U};
static int32_t s_encoder_step_accum = 0;
static int32_t s_encoder_pending_steps = 0;
static bool s_encoder_ready = false;
static bool s_beep_active = false;
static uint32_t s_beep_off_tick = 0U;

static const key_map_t s_key_map[KEY_COUNT] = {
    {KEY1_GPIO_Port, KEY1_Pin, LV_KEY_PREV},
    {KEY2_GPIO_Port, KEY2_Pin, LV_KEY_NEXT},
    {KEY3_GPIO_Port, KEY3_Pin, LV_KEY_ENTER},
    {KEY4_GPIO_Port, KEY4_Pin, LV_KEY_ESC},
};

/**********************
 * GLOBAL FUNCTIONS
 **********************/
void lv_port_indev_init(void)
{
    uint32_t i;

    s_key_pending_mask = 0U;
    s_key_latched_mask = 0U;
    for (i = 0U; i < KEY_COUNT; i++)
    {
        s_key_last_tick[i] = 0U;
    }
    s_encoder_step_accum = 0;
    s_encoder_pending_steps = 0;
    s_encoder_ready = false;
    s_beep_active = false;
    s_beep_off_tick = 0U;
    HAL_GPIO_WritePin(BEEF_GPIO_Port, BEEF_Pin, GPIO_PIN_SET);

    if (HAL_TIM_Encoder_Start(&htim8, TIM_CHANNEL_ALL) == HAL_OK)
    {
        __HAL_TIM_SET_COUNTER(&htim8, 0U);
        s_encoder_ready = true;
    }

    indev_keypad = lv_indev_create();
    lv_indev_set_type(indev_keypad, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(indev_keypad, keypad_read);

    indev_group = lv_group_create();
    lv_group_set_default(indev_group);
    lv_group_set_wrap(indev_group, true);
    lv_indev_set_group(indev_keypad, indev_group);
}

void lv_port_indev_set_group(lv_group_t *group)
{
    if ((indev_keypad == NULL) || (group == NULL))
    {
        return;
    }

    lv_indev_set_group(indev_keypad, group);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    uint32_t i;
    uint32_t now = HAL_GetTick();

    for (i = 0U; i < KEY_COUNT; i++)
    {
        if (GPIO_Pin == s_key_map[i].gpio_pin)
        {
            uint32_t elapsed = now - s_key_last_tick[i];
            uint32_t key_bit = (1UL << i);

            if (((s_key_latched_mask & key_bit) == 0U) &&
                (elapsed >= KEY_EVENT_DEBOUNCE_MS) &&
                (HAL_GPIO_ReadPin(s_key_map[i].gpio_port, s_key_map[i].gpio_pin) == GPIO_PIN_RESET))
            {
                s_key_last_tick[i] = now;
                s_key_pending_mask |= key_bit;
                s_key_latched_mask |= key_bit;
            }
            break;
        }
    }
}

/**********************
 * STATIC FUNCTIONS
 **********************/
static void keypad_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    static uint32_t last_key = LV_KEY_ENTER;
    static bool need_release = false;
    uint32_t event_key = 0U;
    bool has_event = false;

    (void)indev;

    buzzer_service();
    key_latch_refresh();
    encoder_poll_and_queue();

    if (need_release)
    {
        data->state = LV_INDEV_STATE_RELEASED;
        data->key = last_key;
        need_release = false;
        return;
    }

    pop_ui_event(&event_key, &has_event);

    if (has_event)
    {
        last_key = event_key;
        data->key = last_key;
        data->state = LV_INDEV_STATE_PRESSED;
        need_release = true;
        buzzer_trigger();
    }
    else
    {
        data->key = last_key;
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

static void pop_ui_event(uint32_t *key_out, bool *valid_out)
{
    uint32_t i;
    uint32_t pending;
    bool found = false;

    if (s_encoder_pending_steps > 0)
    {
        s_encoder_pending_steps--;
        *key_out = LV_KEY_NEXT;
        *valid_out = true;
        return;
    }
    if (s_encoder_pending_steps < 0)
    {
        s_encoder_pending_steps++;
        *key_out = LV_KEY_PREV;
        *valid_out = true;
        return;
    }

    __disable_irq();
    pending = s_key_pending_mask;
    if (pending != 0U)
    {
        for (i = 0U; i < KEY_COUNT; i++)
        {
            if ((pending & (1UL << i)) != 0U)
            {
                s_key_pending_mask &= ~(1UL << i);
                *key_out = s_key_map[i].lv_key;
                found = true;
                break;
            }
        }
    }
    __enable_irq();

    *valid_out = found;
}

static void encoder_poll_and_queue(void)
{
    int32_t delta;

    if (!s_encoder_ready)
    {
        return;
    }

    delta = (int16_t)__HAL_TIM_GET_COUNTER(&htim8);
    if (delta == 0)
    {
        return;
    }

    __HAL_TIM_SET_COUNTER(&htim8, 0U);
#if (ENCODER_DIR_INVERT == 1U)
    delta = -delta;
#endif

    s_encoder_step_accum += delta;

    while (s_encoder_step_accum >= (int32_t)ENCODER_STEP_TICKS)
    {
        if (s_encoder_pending_steps < ENCODER_PENDING_LIMIT)
        {
            s_encoder_pending_steps++;
        }
        s_encoder_step_accum -= (int32_t)ENCODER_STEP_TICKS;
    }

    while (s_encoder_step_accum <= -((int32_t)ENCODER_STEP_TICKS))
    {
        if (s_encoder_pending_steps > -ENCODER_PENDING_LIMIT)
        {
            s_encoder_pending_steps--;
        }
        s_encoder_step_accum += (int32_t)ENCODER_STEP_TICKS;
    }
}

static void key_latch_refresh(void)
{
    uint32_t i;

    __disable_irq();
    for (i = 0U; i < KEY_COUNT; i++)
    {
        uint32_t key_bit = (1UL << i);
        if ((s_key_latched_mask & key_bit) != 0U)
        {
            if (HAL_GPIO_ReadPin(s_key_map[i].gpio_port, s_key_map[i].gpio_pin) == GPIO_PIN_SET)
            {
                s_key_latched_mask &= ~key_bit;
            }
        }
    }
    __enable_irq();
}

static void buzzer_trigger(void)
{
    HAL_GPIO_WritePin(BEEF_GPIO_Port, BEEF_Pin, GPIO_PIN_RESET);
    s_beep_active = true;
    s_beep_off_tick = HAL_GetTick() + BEEP_ON_TIME_MS;
}

static void buzzer_service(void)
{
    if (!s_beep_active)
    {
        return;
    }
    if ((int32_t)(HAL_GetTick() - s_beep_off_tick) >= 0)
    {
        HAL_GPIO_WritePin(BEEF_GPIO_Port, BEEF_Pin, GPIO_PIN_SET);
        s_beep_active = false;
    }
}

#else /* Enable this file at the top */

/* This dummy typedef exists purely to silence -Wpedantic. */
typedef int keep_pedantic_happy;
#endif
