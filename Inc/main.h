/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  ** This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * COPYRIGHT(c) 2019 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
#include "stm32f1xx_hal.h"

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

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define BK_LED_Pin GPIO_PIN_0
#define BK_LED_GPIO_Port GPIOA
#define DB7_Pin GPIO_PIN_1
#define DB7_GPIO_Port GPIOA
#define DB6_Pin GPIO_PIN_2
#define DB6_GPIO_Port GPIOA
#define DB5_Pin GPIO_PIN_3
#define DB5_GPIO_Port GPIOA
#define DB4_Pin GPIO_PIN_4
#define DB4_GPIO_Port GPIOA
#define D3_CS_Pin GPIO_PIN_0
#define D3_CS_GPIO_Port GPIOB
#define D3BUSY_READ_Pin GPIO_PIN_1
#define D3BUSY_READ_GPIO_Port GPIOB
#define EN_Pin GPIO_PIN_10
#define EN_GPIO_Port GPIOB
#define RS_Pin GPIO_PIN_11
#define RS_GPIO_Port GPIOB
#define SH1B_Pin GPIO_PIN_12
#define SH1B_GPIO_Port GPIOB
#define SH1A_Pin GPIO_PIN_13
#define SH1A_GPIO_Port GPIOB
#define SH1A_EXTI_IRQn EXTI15_10_IRQn
#define KS0_Pin GPIO_PIN_14
#define KS0_GPIO_Port GPIOB
#define SH2A_Pin GPIO_PIN_15
#define SH2A_GPIO_Port GPIOB
#define SH2A_EXTI_IRQn EXTI15_10_IRQn
#define SH2B_Pin GPIO_PIN_8
#define SH2B_GPIO_Port GPIOA
#define KS1_Pin GPIO_PIN_11
#define KS1_GPIO_Port GPIOA
#define IFR_Pin GPIO_PIN_12
#define IFR_GPIO_Port GPIOA
#define KS2_Pin GPIO_PIN_15
#define KS2_GPIO_Port GPIOA
#define KS3_Pin GPIO_PIN_3
#define KS3_GPIO_Port GPIOB
#define KS4_Pin GPIO_PIN_4
#define KS4_GPIO_Port GPIOB
#define KS5_Pin GPIO_PIN_5
#define KS5_GPIO_Port GPIOB
#define SCL_Pin GPIO_PIN_6
#define SCL_GPIO_Port GPIOB
#define SDA_Pin GPIO_PIN_7
#define SDA_GPIO_Port GPIOB
#define RST_Pin GPIO_PIN_8
#define RST_GPIO_Port GPIOB
#define RDS_Pin GPIO_PIN_9
#define RDS_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
