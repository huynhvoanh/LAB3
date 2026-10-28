/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2026 STMicroelectronics.
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "input_reading.h"
#include "input_processing.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define MIN_TRAFFIC_DURATION        1U
#define MAX_TRAFFIC_DURATION        99U
#define SEVEN_SEGMENT_DIGIT_TIME_MS 200U
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */
volatile uint16_t trafficTick10ms = 0;
volatile uint32_t timerElapsedMs = 0;

uint8_t trafficState = 0;
uint8_t remainingSeconds = 3;

uint8_t redDuration = 5;
uint8_t amberDuration = 2;
uint8_t greenDuration = 3;
static uint8_t redDurationLocked = 0U;

uint8_t digitBuffer[4] = {0, 3, 0, 1};
uint8_t currentDigit = 0;
uint32_t lastScanMs = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */
void setTrafficLights(uint8_t red1, uint8_t yellow1,
                      uint8_t green1, uint8_t red2,
                      uint8_t yellow2, uint8_t green2);
void display7SEG(int num);
void updateDisplayBuffer(uint8_t firstValue, uint8_t secondValue);
void applyTrafficState(void);
void nextTrafficState(void);
void updateTrafficDisplay(void);
void updateBlinkingLeds(uint8_t mode, uint8_t ledState);
static void disableAllDigits(void);
static void scan7SEG(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void setTrafficLights(uint8_t red1, uint8_t yellow1,
					  uint8_t green1, uint8_t red2,
					  uint8_t yellow2, uint8_t green2)
{
	HAL_GPIO_WritePin(LED_RED1_GPIO_Port, LED_RED1_Pin, red1? GPIO_PIN_SET: GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LED_YELLOW1_GPIO_Port, LED_YELLOW1_Pin, yellow1? GPIO_PIN_SET: GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LED_GREEN1_GPIO_Port, LED_GREEN1_Pin, green1? GPIO_PIN_SET: GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LED_RED2_GPIO_Port, LED_RED2_Pin, red2? GPIO_PIN_SET: GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LED_YELLOW2_GPIO_Port, LED_YELLOW2_Pin, yellow2? GPIO_PIN_SET: GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LED_GREEN2_GPIO_Port, LED_GREEN2_Pin, green2? GPIO_PIN_SET: GPIO_PIN_RESET);
}

void display7SEG(int num)
{
    static const uint8_t code[10][7] = {
        {1,1,1,1,1,1,0}, // 0
        {0,1,1,0,0,0,0}, // 1
        {1,1,0,1,1,0,1}, // 2
        {1,1,1,1,0,0,1}, // 3
        {0,1,1,0,0,1,1}, // 4
        {1,0,1,1,0,1,1}, // 5
        {1,0,1,1,1,1,1}, // 6
        {1,1,1,0,0,0,0}, // 7
        {1,1,1,1,1,1,1}, // 8
        {1,1,1,1,0,1,1}  // 9
    };

    uint16_t pins[7] = {
        a1_Pin, b1_Pin, c1_Pin,
        d1_Pin, e1_Pin, f1_Pin, g1_Pin
    };

    if (num < 0 || num > 9) num = 0;
    for (uint8_t i = 0; i < 7; i++) {
        HAL_GPIO_WritePin(GPIOB, pins[i],
            code[num][i] ? GPIO_PIN_RESET : GPIO_PIN_SET);
    }
}

static void disableAllDigits(void)
{
    HAL_GPIO_WritePin(GPIOA, EN0_Pin | EN1_Pin | EN2_Pin | EN3_Pin, GPIO_PIN_SET);
}

static void scan7SEG(void)
{
    uint32_t now = HAL_GetTick();
    if ((uint32_t)(now - lastScanMs) < SEVEN_SEGMENT_DIGIT_TIME_MS) return;
    lastScanMs = now;

    disableAllDigits();
    display7SEG(digitBuffer[currentDigit]);

    switch (currentDigit) {
        case 0: HAL_GPIO_WritePin(EN0_GPIO_Port, EN0_Pin, GPIO_PIN_RESET); break;
        case 1: HAL_GPIO_WritePin(EN1_GPIO_Port, EN1_Pin, GPIO_PIN_RESET); break;
        case 2: HAL_GPIO_WritePin(EN2_GPIO_Port, EN2_Pin, GPIO_PIN_RESET); break;
        default: HAL_GPIO_WritePin(EN3_GPIO_Port, EN3_Pin, GPIO_PIN_RESET); break;
    }
    currentDigit = (uint8_t)((currentDigit + 1U) % 4U);
}

static uint8_t limitDisplayValue(uint16_t value)
{
    if (value < 1U) return 1U;
    if (value > 99U) return 99U;
    return (uint8_t)value;
}

void updateDisplayBuffer(uint8_t firstValue, uint8_t secondValue)
{
    firstValue = limitDisplayValue(firstValue);
    secondValue = limitDisplayValue(secondValue);

    digitBuffer[0] = firstValue / 10U;
    digitBuffer[1] = firstValue % 10U;
    digitBuffer[2] = secondValue / 10U;
    digitBuffer[3] = secondValue % 10U;
}

void applyTrafficState(void)
{
    switch (trafficState) {
        case 0: setTrafficLights(0, 0, 1, 1, 0, 0); break;
        case 1: setTrafficLights(0, 1, 0, 1, 0, 0); break;
        case 2: setTrafficLights(1, 0, 0, 0, 0, 1); break;
        case 3: setTrafficLights(1, 0, 0, 0, 1, 0); break;
        default:
            trafficState = 0;
            setTrafficLights(0, 0, 1, 1, 0, 0);
            break;
    }
}

void nextTrafficState(void)
{
    trafficState = (uint8_t)((trafficState + 1U) % 4U);
    switch (trafficState) {
        case 0: remainingSeconds = greenDuration; break;
        case 1: remainingSeconds = amberDuration; break;
        case 2:
            remainingSeconds = (redDuration > amberDuration)
                             ? (uint8_t)(redDuration - amberDuration) : 1U;
            break;
        case 3: remainingSeconds = amberDuration; break;
        default: remainingSeconds = greenDuration; break;
    }
    applyTrafficState();
}

void updateTrafficDisplay(void)
{
    uint8_t mode = get_current_mode();
    uint8_t value;

    if (mode == 1U) {
        uint16_t road1Value;
        uint16_t road2Value;

        switch (trafficState) {
            case 0:
                road1Value = remainingSeconds;
                road2Value = (uint16_t)remainingSeconds + amberDuration;
                break;
            case 1:
                road1Value = remainingSeconds;
                road2Value = remainingSeconds;
                break;
            case 2:
                road1Value = (uint16_t)remainingSeconds + amberDuration;
                road2Value = remainingSeconds;
                break;
            case 3:
            default:
                road1Value = remainingSeconds;
                road2Value = remainingSeconds;
                break;
        }

        updateDisplayBuffer(limitDisplayValue(road1Value),
                            limitDisplayValue(road2Value));
        return;
    }

    switch (mode) {
        case 2: value = get_red_edit_value(); break;
        case 3: value = get_amber_edit_value(); break;
        case 4: value = get_green_edit_value(); break;
        default: mode = 1U; value = remainingSeconds; break;
    }

    updateDisplayBuffer(value, mode);
}

void updateBlinkingLeds(uint8_t mode, uint8_t ledState)
{
    switch (mode) {
        case 2: setTrafficLights(ledState, 0, 0, ledState, 0, 0); break;
        case 3: setTrafficLights(0, ledState, 0, 0, ledState, 0); break;
        case 4: setTrafficLights(0, 0, ledState, 0, 0, ledState); break;
        default: break;
    }
}

static uint8_t clampDuration(uint8_t value, uint8_t minimum, uint8_t maximum)
{
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

void setRedDuration(uint8_t value)
{
    redDuration = clampDuration(value,
                                (uint8_t)(2U * MIN_TRAFFIC_DURATION),
                                MAX_TRAFFIC_DURATION);

    if (amberDuration >= redDuration) {
        amberDuration = (uint8_t)(redDuration - MIN_TRAFFIC_DURATION);
    }

    greenDuration = (uint8_t)(redDuration - amberDuration);
    redDurationLocked = 1U;
}
void setAmberDuration(uint8_t value)
{
    uint8_t maximumAmber;

    if (redDurationLocked) {
        maximumAmber = (uint8_t)(redDuration - MIN_TRAFFIC_DURATION);
        amberDuration = clampDuration(value,
                                      MIN_TRAFFIC_DURATION,
                                      maximumAmber);
        greenDuration = (uint8_t)(redDuration - amberDuration);
        return;
    }

    maximumAmber = (uint8_t)(MAX_TRAFFIC_DURATION - greenDuration);
    amberDuration = clampDuration(value, MIN_TRAFFIC_DURATION, maximumAmber);
    redDuration = (uint8_t)(greenDuration + amberDuration);
}
void setGreenDuration(uint8_t value)
{
    uint8_t maximumGreen;

    if (redDurationLocked) {
        maximumGreen = (uint8_t)(redDuration - MIN_TRAFFIC_DURATION);
        greenDuration = clampDuration(value,
                                      MIN_TRAFFIC_DURATION,
                                      maximumGreen);
        amberDuration = (uint8_t)(redDuration - greenDuration);
        return;
    }

    maximumGreen = (uint8_t)(MAX_TRAFFIC_DURATION - amberDuration);
    greenDuration = clampDuration(value, MIN_TRAFFIC_DURATION, maximumGreen);
    redDuration = (uint8_t)(greenDuration + amberDuration);
}

uint8_t isRedDurationLocked(void)
{
    return redDurationLocked;
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  MX_GPIO_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start_IT(&htim2);

  trafficState = 0;
  remainingSeconds = greenDuration;
  applyTrafficState();
  updateTrafficDisplay();

  uint32_t lastBlinkMs = 0;
  uint32_t lastSecondMs = timerElapsedMs;
  uint8_t blinkState = 0;
  uint8_t lastMode = 0;
  /* USER CODE END 2 */

  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	    uint32_t now = timerElapsedMs;

	    fsm_for_input_processing();
	    processDurationButtons();
	    uint8_t mode = get_current_mode();

	    updateTrafficDisplay();
	    scan7SEG();

	    if (mode != lastMode) {
	        lastMode = mode;
	        if (mode == 1U) {
	            trafficState = 0U;
	            remainingSeconds = greenDuration;
	            lastSecondMs = now;
	            applyTrafficState();
	            updateTrafficDisplay();
	        } else {
	            lastBlinkMs = now;
	            blinkState = 1U;
	            updateBlinkingLeds(mode, blinkState);
	        }
	    }

	    if (mode == 1U) {
	        while ((uint32_t)(now - lastSecondMs) >= 1000U) {
	            lastSecondMs += 1000U;
	            if (remainingSeconds > 0U) remainingSeconds--;
	            if (remainingSeconds == 0U) nextTrafficState();
	            updateTrafficDisplay();
	        }
	    } else if ((uint32_t)(now - lastBlinkMs) >= 250U) {
	        lastBlinkMs += 250U;
	        blinkState ^= 1U;
	        updateBlinkingLeds(mode, blinkState);
	    }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 7999;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 9;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LED_RED1_Pin|LED_YELLOW1_Pin|LED_GREEN1_Pin|LED_RED2_Pin
                          |LED_YELLOW2_Pin|LED_GREEN2_Pin|EN0_Pin|EN1_Pin
                          |EN2_Pin|EN3_Pin, GPIO_PIN_RESET);

  HAL_GPIO_WritePin(GPIOB, a1_Pin|b1_Pin|c1_Pin|d1_Pin
                          |e1_Pin|f1_Pin|g1_Pin, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = LED_RED1_Pin|LED_YELLOW1_Pin|LED_GREEN1_Pin|LED_RED2_Pin
                          |LED_YELLOW2_Pin|LED_GREEN2_Pin|EN0_Pin|EN1_Pin
                          |EN2_Pin|EN3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = a1_Pin|b1_Pin|c1_Pin|d1_Pin
                          |e1_Pin|f1_Pin|g1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = MODE_Pin|TDUR_Pin|VALUE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
