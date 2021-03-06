#ifndef __DATAQUEUES_H__
#define __DATAQUEUES_H__


#include "stdint.h"

typedef enum DataType
{
	DATA_TYPE_BITS 		= 0x0000,		// Bits, not grouped into integers
	DATA_TYPE_I8 		= 0x0000,		// 8-bit Integer signed, full range 0 - 255 (-128 +127)
	DATA_TYPE_Q7		= 0x0100,		// Q7 Fixed Point signed (-1.0 +1.0)
	DATA_TYPE_I16		= 0x0400,		// 16-bit Integer signed, full range (-32768 +32768)
	DATA_TYPE_Q15   	= 0x0500,		// Q15 Fixed Point signed, range (-1.0 +1.0)
	DATA_TYPE_I24		= 0x0800,		// 24-bit Integer signed, full range
	DATA_TYPE_Q23		= 0x0900,   	// Q23 Fixed Point signed, range (-1.0 +1.0)
	DATA_TYPE_I32		= 0x0C00,		// 32-bit Integer signed
	DATA_TYPE_Q31   	= 0x0D00,		// Q31 signed, range (-1.0 + 1.0)
	DATA_TYPE_F32K		= 0x0E00,		// 32-bit Floating point, range (-32K.0 +32K.0)
	DATA_TYPE_F32		= 0x0F00,		// 32-bit Floating point, range (-1.0 +1.0)
// Variuos masks to extract needed info from the type
	DATA_TYPE_MASK		= 0x0F00,
	DATA_SIZE_MASK		= 0x00FF,
	DATA_RANGE_MASK		= 0x0100,		// Range is (-1.0 +1.0) - can be Fixed or Floating
	DATA_FP_MASK		= 0x0200,		// Floating Point representation
	DATA_TYPE_SHIFT   	= 10			// Shift amount to extract data type
}DataType_t;

typedef enum DataChannels
{
// Number of channels in one element
	DATA_NUM_CH_NONE 	= 0x0000,
	DATA_NUM_CH_1 		= 0x0000,
	DATA_NUM_CH_2 		= 0x1000,
	DATA_NUM_CH_3 		= 0x2000,
	DATA_NUM_CH_4 		= 0x3000,
	DATA_NUM_CH_5 		= 0x4000,
	DATA_NUM_CH_6 		= 0x5000,
	DATA_NUM_CH_7 		= 0x6000,
	DATA_NUM_CH_8 		= 0x7000,

//  The data organization when more than one channel is specified
	DATA_ALT 			= 0x0000,		// If more than 1 channel, the elements are interleaved/alternating  ABCDABCDABCD in memory
	DATA_SEQ			= 0x8000,		// If more than 1 channel, all the elements of one channel follow all of another AAABBBBBBDDD in memory
//	The masks to extract the information from the channels data
	DATA_CH_MASK		= 0x7000,
	DATA_SEQ_MASK 		= 0x8000,
	DATA_CH_SHIFT 		= 12			// Shift amount to extract number of channels
} DataChannels_t;

#define DATA_TYPE_SIZE(a)  			((((a) & DATA_TYPE_MASK) >> DATA_TYPE_SHIFT) + 1)
#define DATA_TYPE_NUM_CHANNELS(a) 	((((a) & DATA_CH_MASK) >> DATA_CH_SHIFT) + 1)
#define DATA_ELEM_SIZE(a)  			((a) & DATA_SIZE_MASK)

typedef enum DataChannelsMask
{
	DATA_CHANNEL_ANY 	= 0x0000,
	DATA_CHANNEL_1 		= 0x0001,
	DATA_CHANNEL_2 		= 0x0002,
	DATA_CHANNEL_3 		= 0x0004,
	DATA_CHANNEL_4 		= 0x0008,
	DATA_CHANNEL_5 		= 0x0010,
	DATA_CHANNEL_6 		= 0x0020,
	DATA_CHANNEL_7 		= 0x0040,
	DATA_CHANNEL_8 		= 0x0080,
	DATA_CHANNEL_ALL	= 0x00FF
} DataChannelsMask_t;

typedef struct DBuffer {
	uint16_t	Type;
	uint16_t	Size;			// Total size of the buffer/queue in bytes (must be a multiple of ElemSize)
	uint8_t		*pBuffer;		// Pointer to the actual data storage for the Buffer
} DBuffer_t;

typedef struct DQueue {
	uint16_t	Type;
	uint16_t	Size;		// Total size of the buffer/queue in bytes (must be a multiple of ElemSize)
	uint8_t		*pBuffer;	// Pointer to the actual data storage for the queue
	uint16_t	iGet;		// Get Index
	uint16_t	iPut;		// Put Index
	int32_t		isReady;	// == 0 if number of bytes is less than half. Reset when buffer is underrun
	struct DQueue *pNext;	// Pointer to the next queue (we support single writer/multiple readers)
} DQueue_t;

// The structure that specifies the information about In/Out Data Port
// It may be used to allocate queues connecting different modules
typedef struct DataPort {
	uint16_t	Type;
	uint16_t	Size;	// Required size in bytes (must be a multiple of ElemSize)
} DataPort_t;


extern DQueue_t *Queue_Create(uint32_t nBuffSize, uint32_t type);
extern void 	Queue_Init(DQueue_t *pQueue, uint32_t type);
extern uint32_t Queue_Count(DQueue_t *pQueue);
extern uint32_t Queue_Space(DQueue_t *pQueue);
extern void 	Queue_Clear(DQueue_t *pQueue);
extern uint32_t Queue_Push(DQueue_t *pQueue, void *pDataSrc, uint32_t nBytes);
extern uint32_t Queue_Pop(DQueue_t *pQueue, void *pDataDst, uint32_t nBytes);

extern void 	DataConvert(void *pSrc, uint32_t SrcType, uint32_t SrcChMask,
					void *pDst, uint32_t DstType, uint32_t DstChMask,
						uint32_t *pnSrcBytes, uint32_t *pnDstBytes);

typedef void 	*Create_t(uint32_t Params);
typedef void 	Open_t(void *pHandle, uint32_t Params);
typedef void 	Info_t(void *pHandle, DataPort_t *pDataIn, DataPort_t *pDataOut);
typedef void 	Process_t(void *pHandle, void *pIn, void *pOut, uint32_t *pInBytes, uint32_t *pOutBytes);
typedef void 	Close_t(void *pHandle);

typedef struct ProcessBlock {
	Create_t		*Create;
	Open_t			*Open;
	Info_t			*Info;
	Process_t  		*Process;
	Close_t			*Close;
} ProcessBlock_t;

#endif // __DATAQUEUES_H__
