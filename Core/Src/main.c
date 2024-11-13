/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "dbger.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define RDP_TEST	(1)
#define WRP_TEST	(2)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/**
 *	@brief	set Flash RDP state
 * 	@param	level ref FLASHEx_OB_Read_Protection in stm32f1xx_hal_flash_ex.h
 */
void set_flash_read_protect(uint8_t level)
{
	HAL_StatusTypeDef hal_sta = HAL_OK;
	FLASH_OBProgramInitTypeDef OBInit = {0};
	
	HAL_FLASHEx_OBGetConfig(&OBInit);
	LOG_DBG("set RDP(L%c) to RDP(L%c)\n", (OBInit.RDPLevel==OB_RDP_LEVEL_0)?'0':'1', (level==OB_RDP_LEVEL_0)?'0':'1');
	
	if(OBInit.RDPLevel == OB_RDP_LEVEL_0 && level != OB_RDP_LEVEL_0) {
		// Flash RDP is disable, need enable it
		OBInit.OptionType = OPTIONBYTE_RDP;
		OBInit.RDPLevel = OB_RDP_LEVEL_1;	// any value except 0xA5 is OK
		HAL_FLASH_Unlock();
		HAL_FLASH_OB_Unlock();
		hal_sta = HAL_FLASHEx_OBProgram(&OBInit);
		if(hal_sta == HAL_OK) {
			LOG_DBG("Enable Flash RDP OK, must Power-On Reset to run the program!\n");	// block output
			HAL_FLASH_OB_Launch();
		} else {
			LOG_ERR("Enable Flash RDP err[%d]\n", hal_sta); 
		}
	} else if(OBInit.RDPLevel != OB_RDP_LEVEL_0 && level == OB_RDP_LEVEL_0) {
		// Flash RDP is enable, need disable it
		LOG_DBG("Disable Flash RDP, the Flash will be Erase.\n");
		LOG_DBG("Because the Flash be Erase, you must download the program again\n");
		OBInit.OptionType = OPTIONBYTE_RDP;
		OBInit.RDPLevel = OB_RDP_LEVEL_0;
		HAL_FLASH_Unlock();
		HAL_FLASH_OB_Unlock();
		hal_sta = HAL_FLASHEx_OBProgram(&OBInit);
		if(hal_sta != HAL_OK) {
			LOG_ERR("Disable Flash RDP err[%d]\n", hal_sta);
		}
	}
}

/**
 *	@brief	set Flash pages WRP state
 * 	@param	pages	ref FLASHEx_OB_Write_Protection in stm32f1xx_hal_flash_ex.h
 *	@param	wrpSta	ref FLASHEx_OB_WRP_State in stm32f1xx_hal_flash_ex.h
 *	@note	each bit in WRPPage means one page: 1 is mean this page's WRP is disable, and 0 is mean enable
 */
void set_flash_write_protection(uint32_t pages, uint32_t wrpSta)
{
	HAL_StatusTypeDef hal_sta = HAL_OK;
	FLASH_OBProgramInitTypeDef OBInit = {0};
	
	HAL_FLASHEx_OBGetConfig(&OBInit);
	LOG_DBG("pagesSta[0x%08X](1=WRP-OFF), change to WRP-%s pages[0x%08X]\n", OBInit.WRPPage, wrpSta?"ON":"OFF", pages);
	
	if((OBInit.WRPPage & pages) != 0 && wrpSta == OB_WRPSTATE_ENABLE) {
		OBInit.OptionType = OPTIONBYTE_WRP;
		OBInit.WRPState = OB_WRPSTATE_ENABLE;
		OBInit.WRPPage = pages;
		HAL_FLASH_Unlock();
		HAL_FLASH_OB_Unlock();
		hal_sta = HAL_FLASHEx_OBProgram(&OBInit);
		if(hal_sta == HAL_OK) {
			LOG_DBG("set Flash WRP OK\n");
			// HAL_Delay(20);		// LOG_DBG() is blocking, no need delay
			HAL_FLASH_OB_Launch();	// RESET is enough, no need POR
		} else {
			LOG_ERR("set Flash WRP err[%d]\n", hal_sta);
		}
	} else if((OBInit.WRPPage & pages) != pages && wrpSta == OB_WRPSTATE_DISABLE) {
		OBInit.OptionType = OPTIONBYTE_WRP;
		OBInit.WRPState = OB_WRPSTATE_DISABLE;
		OBInit.WRPPage = pages;
		HAL_FLASH_Unlock();
		HAL_FLASH_OB_Unlock();
		hal_sta = HAL_FLASHEx_OBProgram(&OBInit);
		if(hal_sta == HAL_OK) {
			LOG_DBG("set Flash WRP OK\n");	
			// HAL_Delay(20);		// LOG_DBG() is blocking, no need delay
			HAL_FLASH_OB_Launch();	// RESET is enough, no need POR
		} else {
			LOG_ERR("set Flash WRP err[%d]\n", hal_sta);
		}
	}
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

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
	LOG_DBG("\n\nSTM32 RDP WRP SDRAM_startup demo\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  uint8_t buf[32] = {0};
  while (1)
  {	  
	  // 01 A5				--> RDP OFF
	  // 01 00				--> RDP ON
	  // 02 01 00 00 00 00	--> set page0 WRP OFF
	  // 02 01 00 00 00 01	--> set page0 WRP ON
	  // 02 FF FF FF FF 00	--> set all pages WRP OFF
	  // 02 FF FF FF FF 01	--> set all pages WRP ON
	  HAL_UART_Receive(&huart1, buf, 6, 500);
	  if(buf[0] == RDP_TEST) {
		  set_flash_read_protect(buf[1]);
		  memset(buf, 0, sizeof(buf));
	  } else if(buf[0] == WRP_TEST) {
		  set_flash_write_protection(*(uint32_t*)&buf[1], buf[5]);
		  memset(buf, 0, sizeof(buf));
	  }
	  // LOG_DBG("LED toggle...\n");
	  HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
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
