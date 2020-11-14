/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LED_Pin GPIO_PIN_13
#define LED_GPIO_Port GPIOC
#define BEEPER_PIN_Pin GPIO_PIN_15
#define BEEPER_PIN_GPIO_Port GPIOC
#define SPI_MPU_SS_Pin GPIO_PIN_4
#define SPI_MPU_SS_GPIO_Port GPIOA
#define SPI2_CLK_Pin GPIO_PIN_5
#define SPI2_CLK_GPIO_Port GPIOA
#define SPI2_MISO_Pin GPIO_PIN_6
#define SPI2_MISO_GPIO_Port GPIOA
#define SPI2_MOSI_Pin GPIO_PIN_7
#define SPI2_MOSI_GPIO_Port GPIOA
#define VOLTAGE_DIVIDER_Pin GPIO_PIN_0
#define VOLTAGE_DIVIDER_GPIO_Port GPIOB
#define CURRENT_METER_ADC_PIN_Pin GPIO_PIN_1
#define CURRENT_METER_ADC_PIN_GPIO_Port GPIOB
#define RX_SPI_BIND_PIN_Pin GPIO_PIN_2
#define RX_SPI_BIND_PIN_GPIO_Port GPIOB
#define ESC1_Pin GPIO_PIN_10
#define ESC1_GPIO_Port GPIOB
#define RX_SPI_EXTI_PIN_Pin GPIO_PIN_14
#define RX_SPI_EXTI_PIN_GPIO_Port GPIOA
#define RX_SPI_EXTI_PIN_EXTI_IRQn EXTI15_10_IRQn
#define RX_NSS_PIN_Pin GPIO_PIN_15
#define RX_NSS_PIN_GPIO_Port GPIOA
#define SPI3_SCK_PIN_Pin GPIO_PIN_3
#define SPI3_SCK_PIN_GPIO_Port GPIOB
#define SPI3_MISO_PIN_Pin GPIO_PIN_4
#define SPI3_MISO_PIN_GPIO_Port GPIOB
#define SPI3_MOSI_PIN_Pin GPIO_PIN_5
#define SPI3_MOSI_PIN_GPIO_Port GPIOB
#define ESC2_Pin GPIO_PIN_6
#define ESC2_GPIO_Port GPIOB
#define ESC3_Pin GPIO_PIN_7
#define ESC3_GPIO_Port GPIOB
#define ESC4_Pin GPIO_PIN_8
#define ESC4_GPIO_Port GPIOB
#define RX_SPI_LED_PIN_Pin GPIO_PIN_9
#define RX_SPI_LED_PIN_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
