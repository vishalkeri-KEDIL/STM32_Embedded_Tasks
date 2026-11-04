#include "machine_life.h"
#include "flash_memory.h"
#include <stdio.h>
#include <string.h>

extern TIM_HandleTypeDef htim2;
extern UART_HandleTypeDef huart2;

#define LIFE_SECONDS    (8000UL * 3600UL)         /* 8000 h */
#define TICKS_PER_SEC   10000UL
#define SAVE_EVERY_SEC   600UL        /* save the life every 10 minutes while machine is ON */

volatile uint32_t machine_life_time_sec  = LIFE_SECONDS;
volatile uint32_t machine_on_counter = 0;     /* counter value when machine turned ON */

volatile uint32_t machine_off_counter   = 0;     /* counter value when machine turned OFF */

volatile uint8_t  machine_is_on  = 0;

uint32_t last_print_count        = 0;
volatile uint8_t start_locked = 0;      /* 1 = switch was ON at power-up, start not allowed */
volatile uint8_t save_request = 0;      /* 1 = please save the life */
char uart_text[80];

uint32_t seconds_since_save = 0;      /* ON seconds counted since the last save */


void machine_life_init(void)
{
  HAL_TIM_Base_Start(&htim2);                                  /* start the stopwatch */
  last_print_count = __HAL_TIM_GET_COUNTER(&htim2);

  if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET)   /* switch ON at power-up */
  {
    start_locked = 1;
  }

  machine_life_time_sec = life_load(LIFE_SECONDS);                     /* read the saved life */

  if (*(volatile uint64_t *)LIFE_PAGE_ADDR == 0xFFFFFFFFFFFFFFFFULL)   /* first time only */
  {
    life_save(LIFE_SECONDS);                                           /* write 8000 h once */
  }
}


void machine_life_task(void)
{
  if (save_request == 1)
  {
    save_request = 0;
    life_save(machine_life_time_sec);                                /* save after switch OFF */
    seconds_since_save = 0;                                          /* start the 10 minutes again */
  }

  uint32_t timer_now = __HAL_TIM_GET_COUNTER(&htim2);   /* read the counter register value */

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, machine_is_on ? GPIO_PIN_SET : GPIO_PIN_RESET);   /* LED */

  if ((uint32_t)(timer_now - last_print_count) >= TICKS_PER_SEC)   /* every 1 second */
  {
    last_print_count = last_print_count + TICKS_PER_SEC;

    if (machine_is_on == 1)
    {
      uint32_t machine_on_time_sec = (__HAL_TIM_GET_COUNTER(&htim2) - machine_on_counter) / TICKS_PER_SEC;
      machine_on_counter = machine_on_counter + machine_on_time_sec * TICKS_PER_SEC;      /* ON counter moves forward */
      machine_life_time_sec = (machine_on_time_sec >= machine_life_time_sec) ? 0 : machine_life_time_sec - machine_on_time_sec;

      seconds_since_save = seconds_since_save + machine_on_time_sec;

      if (seconds_since_save >= SAVE_EVERY_SEC)                      /* 10 minutes done */
      {
        seconds_since_save = 0;
        life_save(machine_life_time_sec);                            /* save to Flash */
      }
    }

    uint32_t life_copy = machine_life_time_sec;

    uint8_t  machine_copy   = machine_is_on;

    if (life_copy == 0)
    {
      snprintf(uart_text, sizeof(uart_text), "LIFE OVER - service needed\r\n");
    }

    else
    {
      snprintf(uart_text, sizeof(uart_text),"Machine %s | Life left: %4lu h %02lu min %02lu sec\r\n",machine_copy ? "ON " : "OFF",
               life_copy / 3600UL,
               (life_copy % 3600UL) / 60UL,
               life_copy % 60UL);
    }
    HAL_UART_Transmit(&huart2, (uint8_t*)uart_text, strlen(uart_text), 100);
  }
}


void machine_life_switch_event(void)
{
  uint8_t switch_now = (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET);

  if (switch_now == 0)
  {
    start_locked = 0;                        /* switch seen OFF, unlock */
  }

  if (switch_now == 1 && machine_is_on == 0 && start_locked == 0)    /* switch ON, start allowed */
  {
    machine_on_counter = __HAL_TIM_GET_COUNTER(&htim2);
    machine_is_on  = 1;
  }
  else if (switch_now == 0 && machine_is_on == 1)        /* machine turned OFF */
  {
    machine_off_counter = __HAL_TIM_GET_COUNTER(&htim2);

    uint32_t machine_on_time_sec = (machine_off_counter - machine_on_counter) / TICKS_PER_SEC;

    machine_life_time_sec = (machine_on_time_sec >= machine_life_time_sec) ? 0 : machine_life_time_sec - machine_on_time_sec;

    machine_is_on = 0;
    save_request = 1;
  }
}
