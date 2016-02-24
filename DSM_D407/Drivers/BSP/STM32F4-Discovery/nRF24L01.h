#ifndef __NRF24L01_H
#define __NRF24L01_H

#include <stdint.h>

/* Link functions for nRF24L01+ peripheral */
void		NRF24L01_Init(void *pSPIHandle);
uint8_t		NRF24L01_Write(uint8_t WriteReg, uint8_t *pBuffer,  uint32_t NumBytesToWrite);
uint8_t		NRF24L01_StartWrite(uint8_t WriteReg, uint8_t *pBuffer,  uint32_t NumBytesToWrite);
uint8_t		NRF24L01_Read(uint8_t ReadReg, uint8_t *pBuffer,  uint32_t NumBytesToRead);
uint8_t		NRF24L01_WriteByte(uint8_t WriteReg, uint8_t DataByte);
uint8_t		NRF24L01_ReadByte(uint8_t ReadReg);
uint8_t		NRF24L01_CmdByte(uint8_t Cmd);
void 		NRF24L01_CE(uint32_t newState);
/**
*  Callback when the packet is transmitted
*  We may add another packed into the Tx queue
*/   
void NRF24L01_TxDone_CallBack(void);

/**
*  Callback when the packet is failed to be transmitted
*  We should do something
*/   
void NRF24L01_TxFail_CallBack(void);

/**
*  Callback when the payload packet is received
*  We may add another packed into the Tx queue
*/   
void NRF24L01_RxReady_CallBack(void);

/* Basic Radio functions for nRF24L01+ radio module   */


#endif
