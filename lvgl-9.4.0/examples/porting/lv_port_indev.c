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
#include "stm32h7xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

/*********************
 * DEFINES
 *********************/
#define KEY_EVENT_DEBOUNCE_MS 30U
#define KEY_COUNT 4U

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
static void pop_key_event(uint32_t *idx_out, bool *valid_out);
static void key_latch_refresh(void);

/**********************
 * STATIC VARIABLES
 **********************/
static lv_indev_t *indev_keypad = NULL;
static lv_group_t *indev_group = NULL;

static volatile uint32_t s_key_pending_mask = 0U;
static volatile uint32_t s_key_latched_mask = 0U;
static volatile uint32_t s_key_last_tick[KEY_COUNT] = {0U, 0U, 0U, 0U};

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
    uint32_t event_idx = 0U;
    bool has_event = false;

    (void)indev;

    key_latch_refresh();

    if (need_release)
    {
        data->state = LV_INDEV_STATE_RELEASED;
        data->key = last_key;
        need_release = false;
        return;
    }

    pop_key_event(&event_idx, &has_event);

    if (has_event)
    {
        last_key = s_key_map[event_idx].lv_key;
        data->key = last_key;
        data->state = LV_INDEV_STATE_PRESSED;
        need_release = true;
    }
    else
    {
        data->key = last_key;
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

static void pop_key_event(uint32_t *idx_out, bool *valid_out)
{
    uint32_t i;
    uint32_t pending;
    bool found = false;

    __disable_irq();
    pending = s_key_pending_mask;
    if (pending != 0U)
    {
        for (i = 0U; i < KEY_COUNT; i++)
        {
            if ((pending & (1UL << i)) != 0U)
            {
                s_key_pending_mask &= ~(1UL << i);
                *idx_out = i;
                found = true;
                break;
            }
        }
    }
    __enable_irq();

    *valid_out = found;
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

#else /* Enable this file at the top */

/* This dummy typedef exists purely to silence -Wpedantic. */
typedef int keep_pedantic_happy;
#endif
