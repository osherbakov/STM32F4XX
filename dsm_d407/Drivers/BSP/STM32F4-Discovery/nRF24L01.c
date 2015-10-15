#include "stm32f4_discovery.h"

static SPI_HandleTypeDef    SpiHandle;

static void     SPIx_Init(void);
static void     SPIx_MspInit(void);
static uint8_t  SPIx_WriteRead(uint8_t Byte);
static  void    SPIx_Error(void);

/* Link functions for nRF24L01+ peripheral */
void            NRF24L01_Init(void);
void            NRF24L01_ITConfig(void);
void            NRF24L01_Write(uint8_t *pBuffer, uint8_t WriteAddr, uint16_t NumByteToWrite);
void            NRF24L01_Read(uint8_t *pBuffer, uint8_t ReadAddr, uint16_t NumByteToRead);


/******************************* SPI Routines *********************************/

/**
  * @brief  SPIx Bus initialization
  * @param  None
  * @retval None
  */
static void SPIx_Init(void)
{
  if(HAL_SPI_GetState(&SpiHandle) == HAL_SPI_STATE_RESET)
  {
    /* SPI configuration -----------------------------------------------------*/
    SpiHandle.Instance = DISCOVERY_SPIx;
    SpiHandle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
    SpiHandle.Init.Direction = SPI_DIRECTION_2LINES;
    SpiHandle.Init.CLKPhase = SPI_PHASE_1EDGE;
    SpiHandle.Init.CLKPolarity = SPI_POLARITY_LOW;
    SpiHandle.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLED;
    SpiHandle.Init.CRCPolynomial = 7;
    SpiHandle.Init.DataSize = SPI_DATASIZE_8BIT;
    SpiHandle.Init.FirstBit = SPI_FIRSTBIT_MSB;
    SpiHandle.Init.NSS = SPI_NSS_SOFT;
    SpiHandle.Init.TIMode = SPI_TIMODE_DISABLED;
    SpiHandle.Init.Mode = SPI_MODE_MASTER;

    SPIx_MspInit();
    HAL_SPI_Init(&SpiHandle);
  }
}

/**
  * @brief  Sends a Byte through the SPI interface and return the Byte received 
  *         from the SPI bus.
  * @param  Byte: Byte send.
  * @retval The received byte value
  */
static uint8_t SPIx_WriteRead(uint8_t Byte)
{
  uint8_t receivedbyte = 0;
  
  /* Send a Byte through the SPI peripheral */
  /* Read byte from the SPI bus */
  if(HAL_SPI_TransmitReceive_DMA(&SpiHandle, (uint8_t*) &Byte, (uint8_t*) &receivedbyte, 1) != HAL_OK)
  {
    SPIx_Error();
  }
  
  return receivedbyte;
}

/**
  * @brief  SPIx error treatment function.
  * @param  None
  * @retval None
  */
static void SPIx_Error(void)
{
  /* De-initialize the SPI communication bus */
  HAL_SPI_DeInit(&SpiHandle);
  
  /* Re-Initialize the SPI communication bus */
  SPIx_Init();
}

/**
  * @brief  SPI MSP Init.
  * @param  hspi: SPI handle
  * @retval None
  */
static void SPIx_MspInit(void)
{
  GPIO_InitTypeDef   GPIO_InitStructure;

  /* Enable the SPI peripheral */
  DISCOVERY_SPIx_CLK_ENABLE();
  
  /* Enable SCK, MOSI and MISO GPIO clocks */
  DISCOVERY_SPIx_GPIO_CLK_ENABLE();
  
  /* SPI SCK, MOSI, MISO pin configuration */
  GPIO_InitStructure.Pin = (DISCOVERY_SPIx_SCK_PIN | DISCOVERY_SPIx_MISO_PIN | DISCOVERY_SPIx_MOSI_PIN);
  GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStructure.Pull  = GPIO_PULLDOWN;
  GPIO_InitStructure.Speed = GPIO_SPEED_MEDIUM;
  GPIO_InitStructure.Alternate = DISCOVERY_SPIx_AF;
  HAL_GPIO_Init(DISCOVERY_SPIx_GPIO_PORT, &GPIO_InitStructure);
}
