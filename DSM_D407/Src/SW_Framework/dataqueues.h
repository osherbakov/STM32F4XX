#ifndef __DATAQUEUES_H__
#define __DATAQUEUES_H__


#include "stdint.h"

typedef enum DataType
{
		DATA_TYPE_BITS 		= 0x0000,		// Bits, no integer values
		DATA_TYPE_I8 			= 0x0000,		// 8-bit Integer, range 0 - 255 (-128 +127)
		DATA_TYPE_Q7			= 0x0100,		// Q7 signed (-1.0 +1.0)
		DATA_TYPE_I16			= 0x0400,		// 16-bit Integer, range (-32768 +32768)
	  DATA_TYPE_Q15   	= 0x0500,		// Q15 signed, range (-1.0 +1.0)
		DATA_TYPE_I24			= 0x0800,		// 24-bit Integer, full range
		DATA_TYPE_Q23			= 0x0900,   // Q23 signed, range (-1.0 +1.0)
		DATA_TYPE_I32			= 0x0C00,		// 32-bit Integer
	  DATA_TYPE_Q31   	= 0x0D00,		// Q31 signed, range (-1.0 + 1.0)
		DATA_TYPE_F32_32K	= 0x0E00,		// 32-bit Floating point, range (-32.0K  + 32.0K)
		DATA_TYPE_F32_1		= 0x0F00,		// 32-bit Floating point, range (-1.0 +1.0)
		DATA_TYPE_MASK		= 0x0F00,
		DATA_RANGE_MASK		= 0x0100,		// Range is limited to (-1.0 +1.0)
		DATA_FP_MASK			= 0x0200,		// Floating Point representation
		DATA_TYPE_SHIFT   = 10
}DataType_t;


typedef enum DataChannels
{
		DATA_CH_NONE 	= 0x0000,
		DATA_CH_1 		= 0x1000,
		DATA_CH_2 		= 0x2000,
		DATA_CH_3 		= 0x3000,
		DATA_CH_4 		= 0x4000,
	  DATA_ALT 			= 0x0000,		// If more than 1 channel, the elements are interleaved/alternating
	  DATA_SEQ			= 0x8000,		// If more than 1 channel, all the elements of one channel follow all of another
		DATA_CH_MASK	= 0x7000,
		DATA_SEQ_MASK = 0x8000,
		DATA_CH_SHIFT = 12
} DataChannels_t;

//
// Definitions:
//   Data type and type_size - the type (char, short, int, fixed, float) and size (in bytes) of a single element
//	 Element - the combination of N data types, as having the same type, where N is number of channels
//   The data types in the element can be INTERLEAVED (CH1 CH2 CH1 CH2 ...) or SEQUENTIAL (CH1 CH1 CH1 ... CH2 CH2 CH2...)

#define DATA_TYPE_SIZE(a)  	((((a) & DATA_TYPE_MASK) >> DATA_TYPE_SHIFT) + 1)
#define DATA_NUM_CHANNELS(a) (((a) & DATA_CH_MASK) >> DATA_CH_SHIFT)
#define DATA_ELEM_SIZE(a)  	(DATA_TYPE_SIZE(a) * DATA_NUM_CHANNELS(a))
#define NUM_ELEMS(n, t) 		(DATA_ELEM_SIZE(t) == 0 ? (n) :  (n) / DATA_ELEM_SIZE(t))

typedef struct DQueue
{
	uint8_t		*pBuffer;
	union {
		uint16_t	Type;
		struct {
			uint8_t		ElemSize;		// Size of the element in bytes
			uint8_t		ElemType;		// Type of the element in queue
		};
	};
	uint16_t	nSize;
	uint16_t	iGet;
	uint16_t	iPut;
} DQueue_t;

extern DQueue_t *Queue_Create(uint32_t nBuffSize, uint32_t type);
extern void Queue_Init(DQueue_t *pQueue, uint32_t type);
extern uint32_t Queue_Count(DQueue_t *pQueue);
extern uint32_t Queue_Space(DQueue_t *pQueue);
extern uint32_t Queue_Count_Elem(DQueue_t *pQueue);
extern uint32_t Queue_Space_Elem(DQueue_t *pQueue);
extern void Queue_Clear(DQueue_t *pQueue);
extern uint32_t Queue_PushData(DQueue_t *pQueue, uint8_t *pDataSrc, uint32_t nBytes);
extern uint32_t Queue_PopData(DQueue_t *pQueue, uint8_t *pDataDst, uint32_t nBytes);


#endif // __DATAQUEUES_H__
