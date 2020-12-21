/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
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
#include "stm32f3xx_hal.h"

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
#define ESC4_Pin GPIO_PIN_2
#define ESC4_GPIO_Port GPIOA
#define ESC3_Pin GPIO_PIN_3
#define ESC3_GPIO_Port GPIOA
#define SPI_MPU_SS_Pin GPIO_PIN_4
#define SPI_MPU_SS_GPIO_Port GPIOA
#define SPI2_CLK_Pin GPIO_PIN_5
#define SPI2_CLK_GPIO_Port GPIOA
#define SPI2_MISO_Pin GPIO_PIN_6
#define SPI2_MISO_GPIO_Port GPIOA
#define SPI2_MOSI_Pin GPIO_PIN_7
#define SPI2_MOSI_GPIO_Port GPIOA
#define RX_SPI_EXTI_PIN_Pin GPIO_PIN_8
#define RX_SPI_EXTI_PIN_GPIO_Port GPIOA
#define RX_SPI_EXTI_PIN_EXTI_IRQn EXTI9_5_IRQn
#define RX_SPI_BIND_PIN_Pin GPIO_PIN_9
#define RX_SPI_BIND_PIN_GPIO_Port GPIOA
#define RX_SPI_LED_PIN_Pin GPIO_PIN_10
#define RX_SPI_LED_PIN_GPIO_Port GPIOA
#define LED_Pin GPIO_PIN_3
#define LED_GPIO_Port GPIOB
#define ESC1_Pin GPIO_PIN_8
#define ESC1_GPIO_Port GPIOB
#define ESC2_Pin GPIO_PIN_9
#define ESC2_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
