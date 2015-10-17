#include "stm32f4_discovery.h"
#include "nRF24L01.h"
#include <string.h>

#define  MAX_PAYLOAD_SIZE	(32)

static  SPI_HandleTypeDef    *pSpiHandle;
static  volatile uint32_t	SPI_inprogress;
static 	uint8_t txBuffer[MAX_PAYLOAD_SIZE + 1];
static 	uint8_t rxBuffer[MAX_PAYLOAD_SIZE + 1];

#define		LOW  	(0)
#define		HIGH	(1)

/******************************* SPI Routines *********************************/
/* NRF24L01 Chip Select functions */
static void NRF24L01_CS(uint32_t newState)
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
	pSpiHandle = (SPI_HandleTypeDef *)pSPI;
	SPI_inprogress = 0;
	NRF24L01_CS(HIGH);
	NRF24L01_CE(LOW);
}


/**
  * @brief  Sends multiple bytes through the SPI interface and return the Bytes received 
  *         from the SPI bus.
  * @param  txBytes: Array of Bytes sent.
  * @param  rxBytes: Array of Bytes received.
  * @retval The status
  */


uint8_t		NRF24L01_Write(uint8_t WriteReg, uint8_t *pBuffer,  uint32_t NumBytesToWrite)
{
	HAL_StatusTypeDef status;
	/* Wait until previous transaction is finished    */
	while(SPI_inprogress) {}; 
	SPI_inprogress = 1;
	txBuffer[0] = WriteReg;
	memcpy(&txBuffer[1], pBuffer, NumBytesToWrite);
	NRF24L01_CS(LOW);		// Drop CSN pin to LOW	
	status = HAL_SPI_TransmitReceive_DMA(pSpiHandle, txBuffer, rxBuffer, NumBytesToWrite + 1);
	if(status != HAL_OK)
	{
		NRF24L01_CS(HIGH);	// Set CSN pin to HIGH	  
		SPI_inprogress = 0;  
	}
	return status;
}

uint8_t		NRF24L01_Read(uint8_t ReadReg, uint8_t *pBuffer,  uint32_t NumBytesToRead)
{	
	HAL_StatusTypeDef status;
	/* Wait until previous transaction is finished    */
	while(SPI_inprogress) {}; 
	SPI_inprogress = 1;
	txBuffer[0] = ReadReg;
	memset(&txBuffer[1], 0xFF, NumBytesToRead);

	NRF24L01_CS(LOW);		// Drop CSN pin to LOW		
	status = HAL_SPI_TransmitReceive_DMA(pSpiHandle, txBuffer, rxBuffer, NumBytesToRead + 1);
	if(status != HAL_OK){
		NRF24L01_CS(HIGH);	// Set CSN pin to HIGH	  
		SPI_inprogress = 0;  
	}else {
		/* Wait until this transaction is finished    */
		while(SPI_inprogress) {};
		memcpy(pBuffer, &rxBuffer[1], NumBytesToRead);
	}
	return status;
}


uint8_t		NRF24L01_WriteByte(uint8_t WriteReg, uint8_t DataByte)
{
	/* Wait until previous transaction is finished    */
	while(SPI_inprogress) {}; 
	SPI_inprogress = 1;
	txBuffer[0] = WriteReg;
	txBuffer[1] = DataByte;

	NRF24L01_CS(LOW);		// Drop CSN pin to LOW	
	HAL_SPI_TransmitReceive(pSpiHandle, txBuffer, rxBuffer, 2, HAL_MAX_DELAY);
	NRF24L01_CS(HIGH);	// Set CSN pin to HIGH	  
	SPI_inprogress = 0;

	return rxBuffer[0];	// Byte 0 will have a status
}
	

uint8_t		NRF24L01_ReadByte(uint8_t ReadReg)
{
	/* Wait until previous transaction is finished    */
	while(SPI_inprogress) {}; 
	SPI_inprogress = 1;
	txBuffer[0] = ReadReg;
	txBuffer[1] = 0xFF;

	NRF24L01_CS(LOW);		// Drop CSN pin to LOW	
	HAL_SPI_TransmitReceive(pSpiHandle, txBuffer, rxBuffer, 2, HAL_MAX_DELAY);
	NRF24L01_CS(HIGH);	// Set CSN pin to HIGH	  
	SPI_inprogress = 0;

	return rxBuffer[1];	// Byte 1 will have the requested value
}

uint8_t		NRF24L01_TouchByte(uint8_t TouchReg)
{
	/* Wait until previous transaction is finished    */
	while(SPI_inprogress) {}; 
	SPI_inprogress = 1;
	txBuffer[0] = TouchReg;

	NRF24L01_CS(LOW);		// Drop CSN pin to LOW	
	HAL_SPI_TransmitReceive(pSpiHandle, txBuffer, rxBuffer, 1, HAL_MAX_DELAY);
	NRF24L01_CS(HIGH);	// Set CSN pin to HIGH	  
	SPI_inprogress = 0;

	return rxBuffer[0];	// Byte 0 will have the status value
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
	if(hspi == pSpiHandle) {
		NRF24L01_CS(HIGH);
		SPI_inprogress = 0;
	}
}

