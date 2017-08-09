/**
  ******************************************************************************
  * File Name          : main.c
  * Date               : 19/02/2015 12:46:46
  * Description        : Main program body
  ******************************************************************************
*/
/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"
#include "usart.h"

#include "stm32f4_discovery.h"
#include "stm32f4_discovery_audio.h"

#define	BUFFER_SIZE	(128)
typedef enum {
	COLLECT_CMD = 0,
	COLLECT_PARAM = 1,
	COLLECT_VALUE_INT = 2,
	COLLECT_VALUE_FRAC = 3,
	DONE
} PARSE_STATE;

typedef struct flex_parser_state_t
{
	// Buffers to keep received/transmitted characters
	uint8_t			RxBuffer[BUFFER_SIZE];	
	uint8_t			TxBuffer[BUFFER_SIZE];	
	//  Rx pointers - if at some point (p_receive == p_process), then we have to reset them to RxBuffer
	uint8_t			*p_receive;		// Pointer to the next available location to receive data
	uint8_t			*p_process;		// Pointer to the next location that will be processed (p_process <= p_receive)
	// Tx pointers  - if at some point (p_output == p_send), then reset both to TxBuffer
	uint8_t			*p_output;		// Tracks the next available position to place the data to be sent
	uint8_t			*p_sent;		// Pointer to the last sent data
} flex_parser_state_t;


osMessageQDef(CMDREADY, 32, uint32_t);
osMessageQId 			CommandIsReady;
flex_parser_state_t		parser_state;

#define DEFINE_CRITICAL(a)  uint32_t 	a
 
#define ENTER_CRITICAL(a)             	\
  do {                                  \
	register uint32_t reg0 __ASM("r0"); \
	  reg0 = __get_PRIMASK();			\
	  __disable_irq();					\
	  a = reg0;							\
  } while(0)
 
#define EXIT_CRITICAL(a)              	\
  do{                                   \
	register uint32_t reg0 __ASM("r0"); \
	reg0 = a;							\
	__set_PRIMASK(reg0);				\
  } while(0)
 

DEFINE_CRITICAL(lock) ;

  
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	uint8_t		ch;
	if(huart == &huart2)
	{

ENTER_CRITICAL(lock);
		ch = *parser_state.p_receive;
		if(parser_state.p_receive == parser_state.p_process &&
			parser_state.p_receive != &parser_state.RxBuffer[0]) 
		{
			parser_state.p_process = parser_state.p_receive = &parser_state.RxBuffer[0];
			parser_state.RxBuffer[0] = ch;
		}
		parser_state.p_receive++;
EXIT_CRITICAL(lock);

		// If the character received ir CR or LF - signal the task to start processing
		if(ch == '\n' || ch == '\r') {
			osMessagePut(CommandIsReady, ch, 0);
		}
		// Start waiting for the next character...
		HAL_UART_Receive_DMA(&huart2, parser_state.p_receive, 1);
	}
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	int32_t		bytes;
	if(huart == &huart2)
	{

ENTER_CRITICAL(lock);
		// Update the pointer with the actual number of bytes transmitted
		parser_state.p_sent += huart->TxXferSize;
		
		// See if we still have to send some data out...
		bytes = parser_state.p_output - parser_state.p_sent; 
		if(bytes == 0) 
		{
			parser_state.p_sent = parser_state.p_output = &parser_state.TxBuffer[0];
		}
EXIT_CRITICAL(lock);
		
		if(bytes) {
			HAL_UART_Transmit_DMA(&huart2, parser_state.p_sent, bytes);
		}
	}
}

static float TestValue;

void GetParam(int Param) 
{
	int nBytes;
	
ENTER_CRITICAL(lock);
	nBytes = sprintf((char *)parser_state.p_output, "P 0 %f\n", TestValue );
	parser_state.p_output += nBytes;
EXIT_CRITICAL(lock);
	
	HAL_UART_Transmit_DMA(&huart2, parser_state.p_sent, nBytes);
}

void SetParam(int Param, float Value)
{
	int nBytes;
	TestValue = Value;
	
ENTER_CRITICAL(lock);
	nBytes = sprintf((char *)parser_state.p_output, "P 0\n");
	parser_state.p_output += nBytes;
EXIT_CRITICAL(lock);
	
	HAL_UART_Transmit_DMA(&huart2, parser_state.p_sent, nBytes);
}

void StartUARTTask(void const * argument)
{
	CommandIsReady = osMessageCreate(osMessageQ(CMDREADY), 0);
	
	// Initialize the Parser structure and pointers
	parser_state.p_process = parser_state.p_receive = &parser_state.RxBuffer[0];
	parser_state.p_output = parser_state.p_sent = &parser_state.TxBuffer[0];
	
	// Start the Reading from UART - the IRQ handler will send the Message CommandIsReady
	HAL_UART_Receive_DMA(&huart2, parser_state.p_receive, 1);

	/* Infinite loop */
	for(;;)
	{
		char	ch;
		int		Param;
		float	Value;
		float	Frac;
		PARSE_STATE		st;
	
		// Wait for the event from IRQ Handler
		osMessageGet(CommandIsReady, osWaitForever);
		
		// Start parsing the command - it can be P NNN<cr> or P NNN=VVVV.FFF<cr>
		st = COLLECT_CMD;
		Param = 0;
		Value = 0;
		while(parser_state.p_process < parser_state.p_receive)
		{
			ch = *parser_state.p_process++;
			switch(st)
			{
				case COLLECT_CMD:
					if(ch == 'P') st = COLLECT_PARAM;
					break;

				case COLLECT_PARAM:
					if(ch == '=') st = COLLECT_VALUE_INT;
					else if(ch == '\n' || ch == '\r') {
						GetParam(Param);
						st = DONE;
					}else if( ch >= '0' && ch <= '9') {
						Param = Param * 10 + (ch - '0');
					}
					break;

				case COLLECT_VALUE_INT:
					if(ch == '\n' || ch == '\r'){ 
						SetParam(Param, Value);
						st = DONE;
					}else if(ch == '.' ) {
						Frac = 0.1f;
						st = COLLECT_VALUE_FRAC;
					}else if( ch >= '0' && ch <= '9') {
						Value = Value * 10.0f + (ch - '0');
					}
					break;
				
				case COLLECT_VALUE_FRAC:
					if(ch == '\n' || ch == '\r'){ 
						SetParam(Param, Value);
						st = DONE;
					}else if( ch >= '0' && ch <= '9') {
						Value = Value + Frac * (ch - '0');
						Frac *= 0.1f;
					}
					break;
					
				case DONE:
					break;
			}
			
		}
	}
}
