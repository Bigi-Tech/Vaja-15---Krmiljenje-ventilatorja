/* USER CODE BEGIN Header */

/**

******************************************************************************

* @file : main.c

* @brief : Main program body

******************************************************************************

* @attention

*

* Copyright (c) 2026 STMicroelectronics.

* All rights reserved.

*

* This software is licensed under terms that can be found in the LICENSE file

* in the root directory of this software component.

* If no LICENSE file comes with this software, it is provided AS-IS.

*

******************************************************************************

*/

/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/

#include "main.h"



/* Private includes ----------------------------------------------------------*/

/* USER CODE BEGIN Includes */

#include <stdbool.h> // za bool vrednosti

#include <string.h>

#include <stdio.h> // za UART pošiljanje

#include <math.h> // za funkcijo log()

/* USER CODE END Includes */



/* Private typedef -----------------------------------------------------------*/

/* USER CODE BEGIN PTD */



/* USER CODE END PTD */



/* Private define ------------------------------------------------------------*/

/* USER CODE BEGIN PD */



/* USER CODE END PD */



/* Private macro -------------------------------------------------------------*/

/* USER CODE BEGIN PM */



/* USER CODE END PM */



/* Private variables ---------------------------------------------------------*/

ADC_HandleTypeDef hadc3;



TIM_HandleTypeDef htim1;



UART_HandleTypeDef huart2;



/* USER CODE BEGIN PV */



/* USER CODE END PV */



/* Private function prototypes -----------------------------------------------*/

void SystemClock_Config(void);

static void MX_GPIO_Init(void);

static void MX_USART2_UART_Init(void);

static void MX_TIM1_Init(void);

static void MX_ADC3_Init(void);

/* USER CODE BEGIN PFP */



/* USER CODE END PFP */



/* Private user code ---------------------------------------------------------*/

/* USER CODE BEGIN 0 */

uint16_t ADCValue;

uint16_t PWMValue;

uint8_t TxData[30];

bool Alarm_OFF = 1;



// Konstante in spremenljivke za izračun temperature (tvoji NTC podatki)

float A = 0.0100606748;

float B = -0.0018292535;

float C = 0.0000164710;

float R1, logR1, T1, T2;

float R2 = 6800.0; // Upornost upora R2 v delilniku (po navodilih)

uint16_t T2int;



// Spremenljivka za časovnik utripanja LED (Naloga 4)

uint32_t previousMillis = 0;

/* USER CODE END 0 */



/**

* @brief The application entry point.

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



/* Initialize all configured peripherals */

MX_GPIO_Init();

MX_USART2_UART_Init();

MX_TIM1_Init();

MX_ADC3_Init();

/* USER CODE BEGIN 2 */

HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);

/* USER CODE END 2 */



/* Infinite loop */

/* USER CODE BEGIN WHILE */

while (1)

{

/* USER CODE END WHILE */



/* USER CODE BEGIN 3 */

uint32_t currentMillis = HAL_GetTick();



// --- 1. UTRIPANJE LED (Naloga 4) s pomočjo SysTick časovnika (2 Hz = menjava vsakih 250 ms) ---

if (Alarm_OFF == 0) {

if (currentMillis - previousMillis >= 250) {

previousMillis = currentMillis;

HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);

}

} else {

// Če alarm ni sprožen, zagotovimo, da je LED ugasnjena

HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);

}



// --- 2. BRANJE IN IZRAČUN (Izvede se vsako 1 sekundo) ---

static uint32_t adcMillis = 0;

if (currentMillis - adcMillis >= 1000) {

adcMillis = currentMillis;



// a. Zagon ADC pretvorbe

HAL_ADC_Start(&hadc3);



// b. Vejitev, ki preverja zapolnjenost pretvorbe

if (HAL_ADC_PollForConversion(&hadc3, 100) == HAL_OK) {



// c. Shranitev vrednosti

ADCValue = HAL_ADC_GetValue(&hadc3);



// d. in e. Pošiljanje podatkov preko UART

HAL_UART_Transmit(&huart2, (uint8_t *)"\r\nADC Value: ", 13, 10);

sprintf((char*)TxData, "%u", ADCValue);

HAL_UART_Transmit(&huart2, TxData, strlen((char*)TxData), 10);



// f. 10 ms zakasnitev znotraj vejitve

HAL_Delay(10);

}



// g. Ustavitev ADC pretvorbe zunaj vejitve

HAL_ADC_Stop(&hadc3);



// f) Izračun temperature

R1 = R2 * (4096.0 / (float)ADCValue - 1.0); // Izračun NTC upora R1

logR1 = log(R1);

T1 = (1.0 / (A + B*logR1 + C*logR1*logR1*logR1)); // v Kelvinih

T2 = T1 - 273.15; // v Celzijih

T2int = (int)T2;



// Pošiljanje temperature preko UART

HAL_UART_Transmit(&huart2, (uint8_t *)" | Temperatura: ", 16, 10);

sprintf((char*)TxData, "%u", T2int);

HAL_UART_Transmit(&huart2, TxData, strlen((char*)TxData), 10);

HAL_UART_Transmit(&huart2, (uint8_t *)" C", 2, 10);



// g) Krmiljenje ventilatorja (PWM)

float calcPWM = 1.3333 * T2 - 33.3333; // Linearna enačba



// Omejitev vrednosti med 0 in 100

if (calcPWM < 0) {

PWMValue = 0;

} else if (calcPWM > 100) {

PWMValue = 100;

} else {

PWMValue = (uint16_t)calcPWM;

}



htim1.Instance->CCR1 = PWMValue; // Zapis duty-cycle vrednosti



// h) Logika za alarm (aktivira se nad 45 stopinj)

if (T2int >= 45 && Alarm_OFF == 1) {

Alarm_OFF = 0; // Sproži alarm (LED bo začela utripati zgoraj)

}

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



/** Configure the main internal regulator output voltage

*/

if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)

{

Error_Handler();

}



/** Initializes the RCC Oscillators according to the specified parameters

* in the RCC_OscInitTypeDef structure.

*/

RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;

RCC_OscInitStruct.HSIState = RCC_HSI_ON;

RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;

RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;

RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;

RCC_OscInitStruct.PLL.PLLM = 1;

RCC_OscInitStruct.PLL.PLLN = 10;

RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;

RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;

RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;

if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)

{

Error_Handler();

}



/** Initializes the CPU, AHB and APB buses clocks

*/

RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK

|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;

RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;

RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;

RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;



if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)

{

Error_Handler();

}

}



/**

* @brief ADC3 Initialization Function

* @param None

* @retval None

*/

static void MX_ADC3_Init(void)

{



/* USER CODE BEGIN ADC3_Init 0 */



/* USER CODE END ADC3_Init 0 */



ADC_ChannelConfTypeDef sConfig = {0};



/* USER CODE BEGIN ADC3_Init 1 */



/* USER CODE END ADC3_Init 1 */



/** Common config

*/

hadc3.Instance = ADC3;

hadc3.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;

hadc3.Init.Resolution = ADC_RESOLUTION_12B;

hadc3.Init.DataAlign = ADC_DATAALIGN_RIGHT;

hadc3.Init.ScanConvMode = ADC_SCAN_DISABLE;

hadc3.Init.EOCSelection = ADC_EOC_SINGLE_CONV;

hadc3.Init.LowPowerAutoWait = DISABLE;

hadc3.Init.ContinuousConvMode = DISABLE;

hadc3.Init.NbrOfConversion = 1;

hadc3.Init.DiscontinuousConvMode = DISABLE;

hadc3.Init.ExternalTrigConv = ADC_SOFTWARE_START;

hadc3.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;

hadc3.Init.DMAContinuousRequests = DISABLE;

hadc3.Init.Overrun = ADC_OVR_DATA_PRESERVED;

hadc3.Init.OversamplingMode = DISABLE;

if (HAL_ADC_Init(&hadc3) != HAL_OK)

{

Error_Handler();

}



/** Configure Regular Channel

*/

sConfig.Channel = ADC_CHANNEL_1;

sConfig.Rank = ADC_REGULAR_RANK_1;

sConfig.SamplingTime = ADC_SAMPLETIME_2CYCLES_5;

sConfig.SingleDiff = ADC_SINGLE_ENDED;

sConfig.OffsetNumber = ADC_OFFSET_NONE;

sConfig.Offset = 0;

if (HAL_ADC_ConfigChannel(&hadc3, &sConfig) != HAL_OK)

{

Error_Handler();

}

/* USER CODE BEGIN ADC3_Init 2 */



/* USER CODE END ADC3_Init 2 */



}



/**

* @brief TIM1 Initialization Function

* @param None

* @retval None

*/

static void MX_TIM1_Init(void)

{



/* USER CODE BEGIN TIM1_Init 0 */



/* USER CODE END TIM1_Init 0 */



TIM_ClockConfigTypeDef sClockSourceConfig = {0};

TIM_MasterConfigTypeDef sMasterConfig = {0};

TIM_OC_InitTypeDef sConfigOC = {0};

TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};



/* USER CODE BEGIN TIM1_Init 1 */



/* USER CODE END TIM1_Init 1 */

htim1.Instance = TIM1;

htim1.Init.Prescaler = 80 - 1;

htim1.Init.CounterMode = TIM_COUNTERMODE_UP;

htim1.Init.Period = 100 - 1;

htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;

htim1.Init.RepetitionCounter = 0;

htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

if (HAL_TIM_Base_Init(&htim1) != HAL_OK)

{

Error_Handler();

}

sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;

if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)

{

Error_Handler();

}

if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)

{

Error_Handler();

}

sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;

sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;

sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;

if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)

{

Error_Handler();

}

sConfigOC.OCMode = TIM_OCMODE_PWM1;

sConfigOC.Pulse = 0;

sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;

sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;

sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;

sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;

sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;

if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)

{

Error_Handler();

}

sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;

sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;

sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;

sBreakDeadTimeConfig.DeadTime = 0;

sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;

sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;

sBreakDeadTimeConfig.BreakFilter = 0;

sBreakDeadTimeConfig.Break2State = TIM_BREAK2_DISABLE;

sBreakDeadTimeConfig.Break2Polarity = TIM_BREAK2POLARITY_HIGH;

sBreakDeadTimeConfig.Break2Filter = 0;

sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;

if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)

{

Error_Handler();

}

/* USER CODE BEGIN TIM1_Init 2 */



/* USER CODE END TIM1_Init 2 */

HAL_TIM_MspPostInit(&htim1);



}



/**

* @brief USART2 Initialization Function

* @param None

* @retval None

*/

static void MX_USART2_UART_Init(void)

{



/* USER CODE BEGIN USART2_Init 0 */



/* USER CODE END USART2_Init 0 */



/* USER CODE BEGIN USART2_Init 1 */



/* USER CODE END USART2_Init 1 */

huart2.Instance = USART2;

huart2.Init.BaudRate = 115200;

huart2.Init.WordLength = UART_WORDLENGTH_8B;

huart2.Init.StopBits = UART_STOPBITS_1;

huart2.Init.Parity = UART_PARITY_NONE;

huart2.Init.Mode = UART_MODE_TX_RX;

huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;

huart2.Init.OverSampling = UART_OVERSAMPLING_16;

huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;

huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

if (HAL_UART_Init(&huart2) != HAL_OK)

{

Error_Handler();

}

/* USER CODE BEGIN USART2_Init 2 */



/* USER CODE END USART2_Init 2 */



}



/**

* @brief GPIO Initialization Function

* @param None

* @retval None

*/

static void MX_GPIO_Init(void)

{

GPIO_InitTypeDef GPIO_InitStruct = {0};

/* USER CODE BEGIN MX_GPIO_Init_1 */

/* USER CODE END MX_GPIO_Init_1 */



/* GPIO Ports Clock Enable */

__HAL_RCC_GPIOC_CLK_ENABLE();

__HAL_RCC_GPIOH_CLK_ENABLE();

__HAL_RCC_GPIOA_CLK_ENABLE();

__HAL_RCC_GPIOB_CLK_ENABLE();



/*Configure GPIO pin Output Level */

HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);



/*Configure GPIO pin : User_EXTI_Pin */

GPIO_InitStruct.Pin = User_EXTI_Pin;

GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;

GPIO_InitStruct.Pull = GPIO_NOPULL;

HAL_GPIO_Init(User_EXTI_GPIO_Port, &GPIO_InitStruct);



/*Configure GPIO pin : LED_Pin */

GPIO_InitStruct.Pin = LED_Pin;

GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;

GPIO_InitStruct.Pull = GPIO_NOPULL;

GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

HAL_GPIO_Init(LED_GPIO_Port, &GPIO_InitStruct);



/* EXTI interrupt init*/

HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);

HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);



/* USER CODE BEGIN MX_GPIO_Init_2 */

/* USER CODE END MX_GPIO_Init_2 */

}



/* USER CODE BEGIN 4 */

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)

{

if(GPIO_Pin == User_EXTI_Pin) {



// Navodilo 1d: Resetiramo alarm ob pogoju, da je temperatura pod kritičnim nivojem

if (T2int < 45) {

// Ukaz za izklop LED diode

HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);



// Zaradi odbojnega efekta tipke dodamo zakasnitev

for(uint32_t i = 0; i < 100000; i++);



// Alarm izklopljen

Alarm_OFF = 1;

}

}

}

/* USER CODE END 4 */



/**

* @brief This function is executed in case of error occurrence.

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



#ifdef USE_FULL_ASSERT

/**

* @brief Reports the name of the source file and the source line number

* where the assert_param error has occurred.

* @param file: pointer to the source file name

* @param line: assert_param error line source number

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
