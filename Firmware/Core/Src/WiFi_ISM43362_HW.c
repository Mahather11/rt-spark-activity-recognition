#include "main.h"
#include <stdio.h>
#include <stdint.h>

#define WIFI_SSN_PIN       GPIO_PIN_12  /* PB12, active low */
#define WIFI_RSTN_PIN      GPIO_PIN_1   /* PE1, active low */
#define WIFI_DATARDY_PIN   GPIO_PIN_0   /* PE0, active high */

void WiFi_ISM43362_Pin_Initialize(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();

  /* PB12 is the active-low chip select. */
  GPIO_InitStruct.Pin = WIFI_SSN_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* PE1 is active-low reset; PE0 is the module's data-ready interrupt. */
  GPIO_InitStruct.Pin = WIFI_RSTN_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = WIFI_DATARDY_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  HAL_NVIC_SetPriority(EXTI0_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  HAL_GPIO_WritePin(GPIOB, WIFI_SSN_PIN, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOE, WIFI_RSTN_PIN, GPIO_PIN_SET);
}

void WiFi_ISM43362_Pin_Uninitialize(void)
{
  HAL_GPIO_DeInit(GPIOB, WIFI_SSN_PIN);
  HAL_GPIO_DeInit(GPIOE, WIFI_RSTN_PIN | WIFI_DATARDY_PIN);
}

void WiFi_ISM43362_Pin_RSTN(uint8_t rstn)
{
  printf("HW Reset called! rstn=%u\r\n", (unsigned int)rstn);
  /* Pack API's rstn=1 means assert reset; the board reset is active low. */
  HAL_GPIO_WritePin(GPIOE, WIFI_RSTN_PIN,
                    rstn ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

void WiFi_ISM43362_Pin_SSN(uint8_t ssn)
{
  /* Pack API's ssn=1 means select; chip select is active low. */
  HAL_GPIO_WritePin(GPIOB, WIFI_SSN_PIN,
                    ssn ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

uint8_t WiFi_ISM43362_Pin_DATARDY(void)
{
  return (HAL_GPIO_ReadPin(GPIOE, WIFI_DATARDY_PIN) == GPIO_PIN_SET) ? 1U : 0U;
}
