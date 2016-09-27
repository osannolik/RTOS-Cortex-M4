/*
 * debug.c
 *
 *  Created on: 23 aug 2015
 *      Author: osannolik
 */

#include "debug.h"

int debug_init(void)
{
  GPIO_InitTypeDef GPIOinitstruct;

  __HAL_RCC_GPIOB_CLK_ENABLE();

  GPIOinitstruct.Speed = GPIO_SPEED_HIGH;
  GPIOinitstruct.Pull = GPIO_NOPULL;
  GPIOinitstruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIOinitstruct.Pin = DBG_LED_PIN;
  HAL_GPIO_Init(DBG_LED_PORT, &GPIOinitstruct);

  // PADS
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  GPIOinitstruct.Speed = GPIO_SPEED_HIGH;
  GPIOinitstruct.Pull = GPIO_NOPULL;
  GPIOinitstruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIOinitstruct.Pin = DBG_PAD1_PIN;
  HAL_GPIO_Init(DBG_PAD1_PORT, &GPIOinitstruct);

  GPIOinitstruct.Speed = GPIO_SPEED_HIGH;
  GPIOinitstruct.Pull = GPIO_NOPULL;
  GPIOinitstruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIOinitstruct.Pin = DBG_PAD2_PIN;
  HAL_GPIO_Init(DBG_PAD2_PORT, &GPIOinitstruct);

  GPIOinitstruct.Speed = GPIO_SPEED_HIGH;
  GPIOinitstruct.Pull = GPIO_NOPULL;
  GPIOinitstruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIOinitstruct.Pin = DBG_PAD3_PIN;
  HAL_GPIO_Init(DBG_PAD3_PORT, &GPIOinitstruct);

  GPIOinitstruct.Speed = GPIO_SPEED_HIGH;
  GPIOinitstruct.Pull = GPIO_NOPULL;
  GPIOinitstruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIOinitstruct.Pin = DBG_PAD4_PIN;
  HAL_GPIO_Init(DBG_PAD4_PORT, &GPIOinitstruct);

  GPIOinitstruct.Speed = GPIO_SPEED_HIGH;
  GPIOinitstruct.Pull = GPIO_NOPULL;
  GPIOinitstruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIOinitstruct.Pin = DBG_PAD5_PIN;
  HAL_GPIO_Init(DBG_PAD5_PORT, &GPIOinitstruct);

  DBG_LED_RESET;
  DBG_PAD1_RESET;
  DBG_PAD2_RESET;
  DBG_PAD3_RESET;
  DBG_PAD4_RESET;
  DBG_PAD5_RESET;

  return 0;
}
