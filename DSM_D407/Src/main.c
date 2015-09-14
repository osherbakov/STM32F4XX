/**
  ******************************************************************************
  * File Name          : main.c
  * Date               : 19/02/2015 12:46:46
  * Description        : Main program body
  ******************************************************************************
*/
/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"
#include "crc.h"
#include "dma.h"
#include "i2c.h"
#include "i2s.h"
#include "rng.h"
#include "usb_device.h"
#include "gpio.h"

#include "stm32f4_discovery.h"
#include "stm32f4_discovery_audio.h"
#include "pdm_filter.h"
#include "usbd_audio_if.h"
#include "datatasks.h"


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
  MX_CRC_Init();
  MX_I2C1_Init();
  MX_I2S2_Init();
  MX_I2S3_Init();
  MX_RNG_Init();

  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();

	/* Initialize all BSP features that we want to use - LEDS, buttons, Audio  */
	BSP_LED_Init(LED3);
	BSP_LED_Init(LED4);
	BSP_LED_Init(LED5);
	BSP_LED_Init(LED6);
	BSP_PB_Init(BUTTON_KEY, BUTTON_MODE_GPIO);
 	BSP_AUDIO_OUT_Init(OUTPUT_DEVICE_HEADPHONE, 85, SAMPLE_FREQ);
  BSP_AUDIO_IN_Init(SAMPLE_FREQ, 16, 1);

	osParams.bStartPlay = 1;
	osParams.audioinMode =  AUDIO_MODE_IN_MIC;
	osParams.audiooutMode =  AUDIO_MODE_OUT_I2S;


  /* Call init function for freertos objects (in freertos.c) */
  MX_FREERTOS_Init();

	/* Allocate and initialize data queues that will be used to pass data between tasks */
	/* Technically, we can do that at the beginning of the task, but the safest way is to allocate them now, before any task is run */
	
	// Queues to pass data to/from USB 
	osParams.USB_Out_data = Queue_Create(MAX_AUDIO_SIZE_BYTES * 3, DATA_TYPE_I16 | DATA_NUM_CH_2);
	osParams.USB_In_data = Queue_Create(MAX_AUDIO_SIZE_BYTES * 3, DATA_TYPE_I16 | DATA_NUM_CH_2);
	
	// Queue to get the data from PDM microphone or I2S PCM data source
	osParams.PCM_In_data = Queue_Create(MAX_AUDIO_SIZE_BYTES * 3, DATA_TYPE_Q15 | DATA_NUM_CH_2);

	// Queue to pass data to the output DAC
	osParams.PCM_Out_data = Queue_Create( MAX_AUDIO_SIZE_BYTES * 3, DATA_TYPE_Q15 | DATA_NUM_CH_2);

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

  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_I2S;
  PeriphClkInitStruct.PLLI2S.PLLI2SN = 192;
  PeriphClkInitStruct.PLLI2S.PLLI2SR = 4;
  HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);

}

void StartDefaultTask(void const * argument)
{
	int buttonState;	// User button State

	buttonState = BSP_PB_GetState(BUTTON_KEY);

	/* Infinite loop */
  for(;;)
  {
    osDelay(200);

		// Check the USER button and switch to the next mode if pressed
		if( BSP_PB_GetState(BUTTON_KEY) != buttonState )
		{
			buttonState = BSP_PB_GetState(BUTTON_KEY);
			if(buttonState == 0)
			{
				if(++osParams.audioinMode > AUDIO_MODE_IN_I2S)
				{
					osParams.audioinMode = AUDIO_MODE_IN_MIC;

					// Add next mode for output
					if((osParams.audiooutMode & AUDIO_MODE_OUT_I2S) == 0)
						osParams.audiooutMode |= AUDIO_MODE_OUT_I2S;
					else if((osParams.audiooutMode & AUDIO_MODE_OUT_USB) == 0)
						osParams.audiooutMode |= AUDIO_MODE_OUT_USB;
					else if((osParams.audiooutMode & AUDIO_MODE_OUT_I2SX) == 0)
						osParams.audiooutMode |= AUDIO_MODE_OUT_I2SX;
					else
						osParams.audiooutMode  = AUDIO_MODE_OUT_NONE;
				}
				// If new mode is selected - restart audio output to be in sync with PDM or USB data
				osParams.bStartPlay = 1;
				Queue_Clear(osParams.PCM_Out_data);
				Queue_Clear(osParams.USB_Out_data);
				Queue_Clear(osParams.USB_In_data);
				BSP_LED_Off(LED3);
				BSP_LED_Off(LED4);
				BSP_LED_Off(LED5);
				BSP_LED_Off(LED6);
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

