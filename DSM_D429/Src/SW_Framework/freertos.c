/**
  ******************************************************************************
  * File Name          : freertos.c
  * Date               : 19/02/2015 12:46:44
  * Description        : Code for freertos applications
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "datatasks.h"

/* Variables -----------------------------------------------------------------*/
//Global variables to be shared across modules
AUDIO_ModeTypeDef  audioMode;	// The current Audio Mode
int buttonState;	// User button State

/* Init FreeRTOS */
void MX_FREERTOS_Init() {
  /* USER CODE BEGIN Init */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 256);
  osThreadDef(pdmInTask, StartDataInPDMTask, osPriorityHigh, 0, 256);
	osThreadDef(i2sOutTask, StartDataOutI2STask, osPriorityHigh, 0, 256);
	osMessageQDef(PDMINDATA, 2, uint32_t);
	osMessageQDef(USBINDATA, 2, uint32_t);
	osMessageQDef(I2SINDATA, 2, uint32_t);
	osMessageQDef(I2SOUTDATA, 2, uint32_t);
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
	// The Thread that will handle input data
  osThreadCreate(osThread(pdmInTask), NULL);
  // The thread that handles the output data
  osThreadCreate(osThread(i2sOutTask), NULL);
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
	dataInPDMMsg = osMessageCreate(osMessageQ(PDMINDATA), 0);
	dataInUSBMsg = osMessageCreate(osMessageQ(USBINDATA), 0);
	dataInI2SMsg = osMessageCreate(osMessageQ(I2SINDATA), 0);
	dataOutI2SMsg  = osMessageCreate(osMessageQ(I2SOUTDATA), 0);
  /* USER CODE END RTOS_QUEUES */
}
