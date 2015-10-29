#include "stm32f4_discovery.h"
#include "nRF24L01.h"
#include "nRF24L01regs.h"
#include "nRF24L01conf.h"
#include "tim.h"


#define  MAX_PAYLOAD_SIZE	(32)

static  SPI_HandleTypeDef    *pSpiHandle;
static  volatile uint32_t	SPI_inprogress;
static 	uint8_t txBuffer[MAX_PAYLOAD_SIZE + 1];
static 	uint8_t rxBuffer[MAX_PAYLOAD_SIZE + 1];
int     rxOK = 0;
int		rxPass;

#define		LOW  	(0)
#define		HIGH	(1)

#define  init_obj(a)		do{__disable_irq();(a) = 0;__enable_irq();}while(0)
#define  lock_obj(a) 		do{__disable_irq();if((a) == 0){(a) = 1; break;}__enable_irq();}while(1)
#define  release_obj(a) 	do{(a) = 0;__enable_irq();}while(0)
#define  keep_obj(a) 		__enable_irq()


/******************************* SPI Routines *********************************/
/* NRF24L01 Chip Select functions */
static inline void NRF24L01_CS(uint32_t newState)
{	
	return HAL_GPIO_WritePin(NRF24_CS_GPIO_PORT, NRF24_CS_PIN, (GPIO_PinState) newState);
}

/* NRF24L01 Chip Enable functions */
void NRF24L01_CE(uint32_t newState)
{	
	return HAL_GPIO_WritePin(NRF24_CE_GPIO_PORT, NRF24_CE_PIN, (GPIO_PinState) newState);
}

/**
  * @brief  NRF24L01 Initialization
  * @param  pSPI: pointer to the SPI  handler for SPI driver that controls the chip
  * @retval None
  */
void NRF24L01_Init(void *pSPI)
{
	init_obj(SPI_inprogress);
	pSpiHandle = (SPI_HandleTypeDef *)pSPI;
	lock_obj(SPI_inprogress);
	NRF24L01_CS(HIGH);
	NRF24L01_CE(LOW);
	release_obj(SPI_inprogress);
}


/**
  * @brief  Sends multiple bytes through the SPI interface and return the Bytes received 
  *         from the SPI bus.
  * @param  txBytes: Array of Bytes sent.
  * @param  rxBytes: Array of Bytes received.
  * @retval The status
  */

uint8_t		NRF24L01_WritePayload(uint8_t WriteReg, uint8_t *pBuffer,  uint32_t NumBytesToWrite)
{
	HAL_StatusTypeDef status;
	/* Wait until previous transaction is finished    */
	lock_obj(SPI_inprogress);
	txBuffer[0] = WriteReg;
	memcpy(&txBuffer[1], pBuffer, NumBytesToWrite);
	NRF24L01_CS(LOW);		// Drop CSN pin to LOW	
	status = HAL_SPI_Transmit_DMA(pSpiHandle, txBuffer, NumBytesToWrite + 1);
	if(status != HAL_OK)
	{
		NRF24L01_CS(HIGH);	// Set CSN pin to HIGH
		release_obj(SPI_inprogress);
	}else
	{
		keep_obj(SPI_inprogress);
	}
	return status;
}


uint8_t		NRF24L01_Write(uint8_t WriteReg, uint8_t *pBuffer,  uint32_t NumBytesToWrite)
{
	HAL_StatusTypeDef status;
	/* Wait until previous transaction is finished    */
	lock_obj(SPI_inprogress);
	txBuffer[0] = WriteReg;
	memcpy(&txBuffer[1], pBuffer, NumBytesToWrite);
	NRF24L01_CS(LOW);		// Drop CSN pin to LOW	
	status = HAL_SPI_Transmit(pSpiHandle, txBuffer, NumBytesToWrite + 1, HAL_MAX_DELAY);
	NRF24L01_CS(HIGH);	// Set CSN pin to HIGH	  
	release_obj(SPI_inprogress);
	return status;
}

uint8_t		NRF24L01_Read(uint8_t ReadReg, uint8_t *pBuffer,  uint32_t NumBytesToRead)
{	
	HAL_StatusTypeDef status;
	/* Wait until previous transaction is finished    */
	lock_obj(SPI_inprogress);
	txBuffer[0] = ReadReg;
	memset(&txBuffer[1], 0xFF, NumBytesToRead);

	NRF24L01_CS(LOW);		// Drop CSN pin to LOW		
	status = HAL_SPI_TransmitReceive(pSpiHandle, txBuffer, rxBuffer, NumBytesToRead + 1, HAL_MAX_DELAY);
	if(status == HAL_OK){
		memcpy(pBuffer, &rxBuffer[1], NumBytesToRead);
	}
	NRF24L01_CS(HIGH);	// Set CSN pin to HIGH	  	
	release_obj(SPI_inprogress);
	return status;
}


uint8_t		NRF24L01_WriteByte(uint8_t WriteReg, uint8_t DataByte)
{
	/* Wait until previous transaction is finished    */
	lock_obj(SPI_inprogress);
	txBuffer[0] = WriteReg;
	txBuffer[1] = DataByte;

	NRF24L01_CS(LOW);		// Drop CSN pin to LOW	
	HAL_SPI_TransmitReceive(pSpiHandle, txBuffer, rxBuffer, 2, HAL_MAX_DELAY);
	NRF24L01_CS(HIGH);	// Set CSN pin to HIGH	  
	release_obj(SPI_inprogress);

	return rxBuffer[0];	// Byte 0 will have a status
}
	

uint8_t		NRF24L01_ReadByte(uint8_t ReadReg)
{
	/* Wait until previous transaction is finished    */
	lock_obj(SPI_inprogress);
	txBuffer[0] = ReadReg;
	txBuffer[1] = 0xFF;

	NRF24L01_CS(LOW);		// Drop CSN pin to LOW	
	HAL_SPI_TransmitReceive(pSpiHandle, txBuffer, rxBuffer, 2, HAL_MAX_DELAY);
	NRF24L01_CS(HIGH);	// Set CSN pin to HIGH	  
	release_obj(SPI_inprogress);
	return rxBuffer[1];	// Byte 1 will have the requested value
}

uint8_t		NRF24L01_CmdByte(uint8_t TouchReg)
{
	/* Wait until previous transaction is finished    */
	lock_obj(SPI_inprogress);
	txBuffer[0] = TouchReg;
	NRF24L01_CS(LOW);		// Drop CSN pin to LOW	
	HAL_SPI_TransmitReceive(pSpiHandle, txBuffer, rxBuffer, 1, HAL_MAX_DELAY);
	NRF24L01_CS(HIGH);		// Set CSN pin to HIGH	  
	release_obj(SPI_inprogress);
	return rxBuffer[0];	// Byte 0 will have the status value
}

/*
 *  Callback when Payload DMA transfer thru SPI is done - activate CE pulse
 */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
	if(hspi == pSpiHandle) {
		NRF24L01_CS(HIGH);
		release_obj(SPI_inprogress);
		// Generate 10-12 usec pulse on CE
		HAL_TIM_Base_Start_IT(&htim10);	
		NRF24L01_CE(HIGH);		
	}
}


/*
 * Callback when 10 usec timer expires - time to drop CE pin
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim == &htim10)
	{
		HAL_TIM_Base_Stop_IT(&htim10);
		__HAL_TIM_SET_COUNTER(&htim10, 0);			
		NRF24L01_CE(LOW);		
	}
}

/*****************************************************************************/
/*              Interrupt callbacks for nRF24L01                             */

void NRF24L01_TxDone_CallBack(void)
{
	
}
void NRF24L01_TxFail_CallBack(void)
{
	
}

extern void RF24_read_payload( void* buf, uint8_t len );
extern void RF24_flushRx(void);
static uint8_t RxBuffer[MAX_PAYLOAD_SIZE];

void NRF24L01_RxReady_CallBack(void)
{
BSP_LED_On(LED6);
//	RF24_read_payload(RxBuffer, 16);
	RF24_flushRx();	
	rxOK++;
}
