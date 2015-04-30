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
osObjects_t osParams;

/* Init FreeRTOS */
void MX_FREERTOS_Init() {
  /* USER CODE BEGIN Init */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityBelowNormal, 0, 256);
  osThreadDef(pdmInTask, StartDataInPDMTask, osPriorityHigh, 0, 256);
	osThreadDef(dataProcessTask, StartDataProcessTask, osPriorityNormal, 0, 2048);
	
	osMessageQDef(PDMINDATA, 2, uint32_t);
	osMessageQDef(DATAREADY, 16, uint32_t);
	
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
  osThreadCreate(osThread(pdmInTask), &osParams);
  // The thread that handles the output data
	// The thread that processes the data
	osThreadCreate(osThread(dataProcessTask), &osParams);
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
	osParams.dataInPDMMsg = osMessageCreate(osMessageQ(PDMINDATA), 0);
	osParams.dataReadyMsg = osMessageCreate(osMessageQ(DATAREADY), 0);
  /* USER CODE END RTOS_QUEUES */
}
