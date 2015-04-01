#ifndef __DATAQUEUES_H__
#define __DATAQUEUES_H__


#include "stdint.h"

typedef struct
{
	uint8_t		*pBuffer;
	union {
		uint16_t	Type;
		struct {
			uint8_t		ElemSize;
			uint8_t		ElemType;
		};
	};
	uint16_t	nSize;
	uint16_t	iGet;
	uint16_t	iPut;
} 
BuffBlk_t;

extern BuffBlk_t *Queue_Create(uint32_t nBuffSize, uint32_t type);
extern void Queue_Init(BuffBlk_t *pQueue, uint32_t nBuffSize, uint32_t type);
extern uint32_t Queue_Count(BuffBlk_t *pQueue);
extern uint32_t Queue_Space(BuffBlk_t *pQueue);
extern void Queue_Clear(BuffBlk_t *pQueue);
extern uint32_t Queue_PushData(BuffBlk_t *pQueue, uint8_t *pData, uint32_t nBytes);
extern uint32_t Queue_PopData(BuffBlk_t *pQueue, uint8_t *pData, uint32_t nBytes);
extern uint32_t Queue_CopyData(BuffBlk_t *pDestQ, BuffBlk_t *pSrcQ, uint32_t nBytes);

#endif // __DATAQUEUES_H__
