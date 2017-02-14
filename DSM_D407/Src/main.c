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
#include "spi.h"
#include "tim.h"

#include "stm32f4_discovery.h"
#include "stm32f4_discovery_audio.h"
#include "pdm_filter.h"
#include "usbd_audio_if.h"
#include "datatasks.h"


/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

//Global variables to be shared across modules
osObjects_t osParams;
osThreadDef(defaultTask, StartDefaultTask, osPriorityBelowNormal, 0, 256);
osThreadDef(pdmInTask, StartDataInPDMTask, osPriorityHigh, 0, 256);
osThreadDef(dataProcessTask, StartDataProcessTask, osPriorityNormal, 0, 2048);
osThreadDef(blinkTask, StartBlinkTask, osPriorityBelowNormal, 0, 256);
osMessageQDef(DATAREADY, 32, uint32_t);

int main(void)
{

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
	MX_SPI1_Init();
	MX_TIM10_Init();
	/* init code for USB_DEVICE */
	MX_USB_DEVICE_Init();

	BSP_RF24_Init(&hspi1);

	osParams.audioInMode =  AUDIO_MODE_IN_MIC;

	/* Allocate and initialize data queues that will be used to pass data between tasks */
	/* Technically, we can do that at the beginning of the task, but the safest way is to allocate them now, before any task is run */

	/* Create the thread(s) */
	osThreadCreate(osThread(defaultTask), NULL);

	// The Thread that will handle input data
	osThreadCreate(osThread(pdmInTask), &osParams);
	// The thread that processes the data
	osThreadCreate(osThread(dataProcessTask), &osParams);

	osThreadCreate(osThread(blinkTask), NULL);

	osParams.dataInReadyMsg = osMessageCreate(osMessageQ(DATAREADY), 0);

	// Queues to pass data to/from USB
	osParams.USB_OutQ = Queue_Create(MAX_AUDIO_SIZE_BYTES * 3, DATA_TYPE_Q15 | DATA_NUM_CH_2);
	osParams.USB_InQ = Queue_Create(MAX_AUDIO_SIZE_BYTES * 3, DATA_TYPE_Q15 | DATA_NUM_CH_2);

	// Queue to get the data from PDM microphone or I2S PCM data source
	osParams.PCM_InQ = Queue_Create(MAX_AUDIO_SIZE_BYTES * 3, DATA_TYPE_Q15 | DATA_NUM_CH_2);

	// Queue to pass data to the output DAC
	osParams.PCM_OutQ = Queue_Create( MAX_AUDIO_SIZE_BYTES * 3, DATA_TYPE_Q15 | DATA_NUM_CH_2);

	// Queues for Upsample and Downsample
	osParams.DownSampleQ = Queue_Create( MAX_AUDIO_SIZE_BYTES * 3, DATA_TYPE_F32 | DATA_NUM_CH_1);
	osParams.UpSampleQ = Queue_Create( MAX_AUDIO_SIZE_BYTES * 3, DATA_TYPE_F32 | DATA_NUM_CH_1);

	// Queue for RateSync'd data
	osParams.RateSyncQ = Queue_Create( MAX_AUDIO_SIZE_BYTES * 3, DATA_TYPE_Q15 | DATA_NUM_CH_2);
	
	// Link output queues
	osParams.PCM_OutQ->pNext = osParams.USB_InQ;
	
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
  PeriphClkInitStruct.PLLI2S.PLLI2SN = 258;
  PeriphClkInitStruct.PLLI2S.PLLI2SR = 3;
  HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);

	//--------------------------------------------------------------------
	// To enable USB and I2S clock sync we have to enable Cycles Counter
	//--------------------------------------------------------------------
	// Enable TRC
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	// Reset the counter
	DWT->CYCCNT = 0;
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void StartDefaultTask(void const * argument)
{
	int buttonState;	// User button State

	/* Initialize all BSP features that we want to use - LEDS, buttons, Audio  */
	BSP_LED_Init(LED3);
	BSP_LED_Init(LED4);
	BSP_LED_Init(LED5);
	BSP_LED_Init(LED6);
	BSP_PB_Init(BUTTON_KEY, BUTTON_MODE_GPIO);

	buttonState = BSP_PB_GetState(BUTTON_KEY);
	osParams.bStartPlay = 1;
	
	/* Infinite loop */
	for(;;)
	{
		osDelay(100);
		// Check the USER button and switch to the next mode if pressed
		if( BSP_PB_GetState(BUTTON_KEY) != buttonState )
		{
			buttonState = BSP_PB_GetState(BUTTON_KEY);
			if(buttonState == 0)
			{
				osParams.audioInMode++;
				if(osParams.audioInMode > AUDIO_MODE_IN_I2SX)	{
					osParams.audioInMode = AUDIO_MODE_IN_MIC;
				}
				// If new mode is selected - restart audio output to be in sync with PDM or USB data
				BSP_AUDIO_OUT_Stop(CODEC_PDWN_SW);
				
				Queue_Clear(osParams.PCM_OutQ);
				Queue_Clear(osParams.USB_OutQ);
				Queue_Clear(osParams.PCM_InQ);
				Queue_Clear(osParams.USB_InQ);
				Queue_Clear(osParams.RateSyncQ);
				
				osParams.bStartPlay = 1;
				
				BSP_LED_Off(LED3);
				BSP_LED_Off(LED4);
				BSP_LED_Off(LED5);
				BSP_LED_Off(LED6);
			}
		}
	}
}


int ALL_LEDS_ON = 0;

void StartBlinkTask(void const * argument)
{
 /* Infinite loop */
  for(;;)
  {
    osDelay(1000);
	if(ALL_LEDS_ON) {
		BSP_LED_Off(LED3);
		BSP_LED_Off(LED4);
		BSP_LED_Off(LED5);
		BSP_LED_Off(LED6);
		ALL_LEDS_ON = 0;
	}else {
		BSP_LED_On(LED3);
		BSP_LED_On(LED4);
		BSP_LED_On(LED5);
		BSP_LED_On(LED6);
		ALL_LEDS_ON = 1;
	}
  }
}

