#ifndef __NRF24L01_H
#define __NRF24L01_H

#include <stdint.h>

/* Link functions for nRF24L01+ peripheral */
void		NRF24L01_Init(void *pSPIHandle);
uint8_t		NRF24L01_Write(uint8_t WriteReg, uint8_t *pBuffer,  uint32_t NumBytesToWrite);
uint8_t		NRF24L01_Read(uint8_t ReadReg, uint8_t *pBuffer,  uint32_t NumBytesToRead);
uint8_t		NRF24L01_WriteByte(uint8_t WriteReg, uint8_t DataByte);
uint8_t		NRF24L01_ReadByte(uint8_t ReadReg);
uint8_t		NRF24L01_TouchByte(uint8_t TouchReg);
void 		NRF24L01_CE(uint32_t newState);

/* Basic Radio functions for nRF24L01+ radio module   */


#endif
