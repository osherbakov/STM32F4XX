/**
  ******************************************************************************
  * File Name          : main.c
  * Date               : 13/02/2015 15:49:13
  * Description        : Main program body
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "i2s.h"
#include "rng.h"
#include "sai.h"
#include "spi.h"
#include "usart.h"
#include "usb_device.h"
#include "gpio.h"

/* USER CODE BEGIN Includes */
#include "stm32f429i_discovery.h"
#include "usbd_audio_if.h"
#include "datatasks.h"
/* USER CODE END Includes */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);

int main(void)
{

  /* MCU Configuration----------------------------------------------------------*/
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();
  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_ADC2_Init();
  MX_I2C3_Init();
  MX_I2S2_Init();
  MX_I2S3_Init();
  MX_RNG_Init();
  MX_SAI1_Init();
  MX_SPI4_Init();
  MX_USART3_UART_Init();

  /* USER CODE BEGIN 2 */
  MX_USB_DEVICE_Init();
	BSP_LED_Init(LED3);
	BSP_LED_Init(LED4);
	BSP_PB_Init(BUTTON_KEY, BUTTON_MODE_GPIO);
	buttonState = BSP_PB_GetState(BUTTON_KEY);
	audioMode = AUDIO_MODE_MIC;
  /* USER CODE END 2 */

  /* Call init function for freertos objects (in freertos.c) */
  MX_FREERTOS_Init();

  /* Start scheduler */
  osKernelStart();
  
  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  while (1)
  {
  }
}

/** System Clock Configuration
*/
void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct;

  __PWR_CLK_ENABLE();

  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1
                              |RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);

  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_I2S|RCC_PERIPHCLK_SAI_PLLSAI;
  PeriphClkInitStruct.PLLI2S.PLLI2SN = 192;
  PeriphClkInitStruct.PLLI2S.PLLI2SR = 2;
  PeriphClkInitStruct.PLLSAI.PLLSAIN = 192;
  PeriphClkInitStruct.PLLSAI.PLLSAIQ = 2;
  PeriphClkInitStruct.PLLSAIDivQ = 8;
  HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);

}

void StartDefaultTask(void const * argument)
{
/* Infinite loop */
  for(;;)
  {
    osDelay(200);

		// Blink proper LEDs
		if(audioMode == AUDIO_MODE_MIC)
		{
			BSP_LED_Toggle(LED3);
		}else if(audioMode == AUDIO_MODE_USB)
		{
			BSP_LED_Toggle(LED4);
		}else
		{
			BSP_LED_Toggle(LED3);
			BSP_LED_Toggle(LED4);
		}

		// Check the USER button and toggle the mode if pressed
		if( BSP_PB_GetState(BUTTON_KEY) != buttonState )
		{
			buttonState = BSP_PB_GetState(BUTTON_KEY);
			if(buttonState == 0)
			{
				if(++audioMode > AUDIO_MODE_I2S) audioMode = AUDIO_MODE_MIC;
				// If new mode is PDM Microphone - restart audio output to be in sync
				if(audioMode == AUDIO_MODE_MIC)
				{
				}
				BSP_LED_Off(LED3);
				BSP_LED_Off(LED4);
			}
		}
  }
}


/**
   * @brief Reports the name of the source file and the source line number
   * where the assert_param error has occurred.
   * @param file: pointer to the source file name
   * @param line: assert_param error line source number
   * @retval None
   */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
//   printf("Wrong parameters value: file %s on line %d\r\n", file, line);
  /* USER CODE END 6 */

}


/**
  * @}
  */ 

/**
  * @}
*/ 
