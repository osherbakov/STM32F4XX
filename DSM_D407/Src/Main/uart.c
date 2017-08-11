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
	register uint32_t reg; 				\
	  reg = __get_PRIMASK();			\
	  __disable_irq();					\
	  a = reg;							\
  } while(0)
 
#define EXIT_CRITICAL(a)              	\
  do{                                   \
	register uint32_t reg; 				\
	reg = a;							\
	__set_PRIMASK(reg);					\
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
	int32_t		nBytes;
	if(huart == &huart2)
	{

ENTER_CRITICAL(lock);
		// Update the pointer with the actual number of bytes transmitted
		parser_state.p_sent += huart->TxXferSize;
		
		// See if we still have to send some data out...
		nBytes = parser_state.p_output - parser_state.p_sent; 
		// If no data to be sent - move all pointers to the beginning of the buffer
		if(nBytes == 0) 
		{
			parser_state.p_sent = parser_state.p_output = &parser_state.TxBuffer[0];
		}
EXIT_CRITICAL(lock);
		
		if(nBytes) {
			HAL_UART_Transmit_DMA(&huart2, parser_state.p_sent, nBytes);
		}
	}
}

static float TestValue;


uint8_t *inttoascii(uint8_t *p_buff, int val) 
{ 
  unsigned char d, i; 
  unsigned char zero; 
  int test; 
  
  zero = 1; 

  i = 9; 
  do{ 
    i--;    
    if ( i==0) 		test =10; 
    else if ( i==1) test =100; 
    else if ( i==2) test =1000; 
    else if ( i==3) test =10000; 
    else if ( i==4) test =100000; 
    else if ( i==5) test =1000000; 
    else if ( i==6) test =10000000; 
    else if ( i==7) test =100000000; 
    else if ( i==8) test =1000000000; 
   
    for( d = '0'; val >= test; val -= test ) 
    { 
      d++; 
      zero = 0; 
    } 
    if( zero == 0 ) 
      *p_buff++ = d; 
  }while( i ); 

  *p_buff++ = (unsigned char)val + '0'; 
  return p_buff;
}


#define MAX_FLOAT_DIGITS  (12)

static uint8_t *floattoascii(uint8_t *p_buff, float data)
{
	unsigned int i;
	unsigned int int_part;					// Integer part of the number
	float	max_fractional;

	max_fractional = 1.0f;
	for(i = 0; i < MAX_FLOAT_DIGITS; i++) max_fractional *= 0.1f;

	// Check and add negative sign (if necessary)
	if(data < 0)
	{
		data = -data;			// Make the number positive
		*p_buff++ = '-';		// Output the sign
	}

	int_part = (unsigned int) data;
	data -= int_part;

	p_buff = inttoascii(p_buff, int_part);

	if (data > max_fractional)
	{
		// Add decimal point
		*p_buff++ = '.';

		// Send the fractional part
		for (; (data > max_fractional); i++)
		{
			data *= 10;
			int_part = (unsigned int)data;
			*p_buff++ = '0' + int_part;
			data -= int_part;
			max_fractional *= 10;
		}
	}
	// *p_buff++ = ' ';	// Add space at the end
	return p_buff;
}


void GetParam(int Param) 
{
	int nBytes;
	
ENTER_CRITICAL(lock);
	*parser_state.p_output++ = 'P';
	*parser_state.p_output++ = ' ';
	*parser_state.p_output++ = '0';
	*parser_state.p_output++ = ' ';
	parser_state.p_output = floattoascii(parser_state.p_output, TestValue);
	*parser_state.p_output++ = '\n';
	nBytes = parser_state.p_output - parser_state.p_sent;
EXIT_CRITICAL(lock);
	
	HAL_UART_Transmit_DMA(&huart2, parser_state.p_sent, nBytes);
}

void SetParam(int Param, float Value)
{
	int nBytes;
	TestValue = Value;
	
ENTER_CRITICAL(lock);
	*parser_state.p_output++ = 'P';
	*parser_state.p_output++ = ' ';
	*parser_state.p_output++ = '0';
	*parser_state.p_output++ = '\n';
	nBytes = parser_state.p_output - parser_state.p_sent;
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
		int		nBytes;
		PARSE_STATE		st;
	
		// Wait for the event from IRQ Handler
		osMessageGet(CommandIsReady, osWaitForever);
		
		// Start parsing the command - it can be P NNN<cr> or P NNN=VVVV.FFF<cr>
		st = COLLECT_CMD;
		Param = 0;
		Value = 0;
		Frac = 0.1f;
		do
		{
ENTER_CRITICAL(lock);
			nBytes = parser_state.p_receive - parser_state.p_process;
			if(nBytes > 0) {
				ch = *parser_state.p_process++;
			}else {
				st = DONE;
			}
EXIT_CRITICAL(lock);
			
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
		} while(st != DONE);
	}
}
