/*
 Copyright (C) 2011 J. Coliz <maniacbug@ymail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

#include "nRF24L01.h"
#include "nRF24L01conf.h"
#include "nRF24L01regs.h"
#include "nRF24L01func.h"

#define RF24_ce(a) NRF24L01_CE(a)

#define MAX_PAYLOAD_SIZE   (32)

#ifndef TRUE 
#define TRUE  (1)
#endif
#ifndef FALSE
#define FALSE (0)
#endif
#ifndef HIGH 
#define HIGH  	(1)
#endif
#ifndef LOW
#define LOW 	(0)
#endif

extern uint32_t HAL_GetTick(void);
extern void		HAL_Delay(uint32_t delay_ms);
extern void 	delay_us(uint32_t delay_microsecs);

#define delay	HAL_Delay
#define millis	HAL_GetTick

/****************************************************************************/

  int 	p_variant; 				/* False for RF24L01 and true for RF24L01P */
  int 	dynamic_payload_enabled; /**< Whether dynamic payloads are enabled. */
  int   ack_payload_enabled;


  uint8_t pipe0_reading_address[5]; /**< Last address set on pipe 0 for reading. */
  uint8_t pipe0_payload_size;
  uint8_t pipe0_enabled;
  
  uint8_t addr_width; 			/**< The address width to use - 3,4 or 5 bytes. */
  uint32_t txRxDelay; 			/**< Var for adjusting delays depending on datarate */
  uint8_t RF24_config;

/****************************************************************************/
uint8_t RF24_read_register_bytes(uint8_t reg, uint8_t* buf, uint8_t len)
{
  return NRF24L01_Read(R_REGISTER | ( REGISTER_MASK & reg ), buf, len);
}

/****************************************************************************/

uint8_t RF24_read_register(uint8_t reg)
{
  return NRF24L01_ReadByte(R_REGISTER | ( REGISTER_MASK & reg ));
}

/****************************************************************************/

uint8_t RF24_write_register_bytes(uint8_t reg, uint8_t* buf, uint8_t len)
{
  return NRF24L01_Write(W_REGISTER | ( REGISTER_MASK & reg ), buf, len);
}

/****************************************************************************/

uint8_t RF24_write_register(uint8_t reg, uint8_t value)
{
  return NRF24L01_WriteByte(W_REGISTER | ( REGISTER_MASK & reg ), value);
}

/****************************************************************************/
uint8_t RF24_write_payload(const void* buf, uint8_t data_len)
{
  return NRF24L01_WritePayload(W_TX_PAYLOAD, (uint8_t *)buf, data_len);
}

uint8_t RF24_write_payload_no_ack(const void* buf, uint8_t data_len)
{
  return NRF24L01_WritePayload(W_TX_PAYLOAD_NO_ACK, (uint8_t *)buf, data_len);
}
/****************************************************************************/

uint8_t RF24_read_payload(void* buf, uint8_t data_len)
{
  return NRF24L01_Read(R_RX_PAYLOAD, buf, data_len);
}

/****************************************************************************/

uint8_t RF24_flushRx(void)
{
  return NRF24L01_CmdByte(FLUSH_RX);
}

/****************************************************************************/

uint8_t RF24_flushTx(void)
{
  return NRF24L01_CmdByte(FLUSH_TX);
}

/****************************************************************************/

uint8_t RF24_getStatus(void)
{
  return NRF24L01_CmdByte(NOP);
}

/****************************************************************************/
#if !defined (MINIMAL)
void RF24_print_status(uint8_t status)
{
  printf_P(PSTR("STATUS\t\t = 0x%02x RX_DR=%x TX_DS=%x MAX_RT=%x RX_P_NO=%x TX_FULL=%x\r\n"),
	status,
	(status & _BV(RX_DR))?1:0,
	(status & _BV(TX_DS))?1:0,
	(status & _BV(MAX_RT))?1:0,
	((status >> RX_P_NO) & 0x07),
	(status & _BV(TX_FULL))?1:0
	);
}

/****************************************************************************/

void RF24_print_byte_register(const char* name, uint8_t reg, uint8_t qty)
{
  //char extra_tab = strlen_P(name) < 8 ? '\t' : 0;
  //printf_P(PSTR(PRIPSTR"\t%c ="),name,extra_tab);
  printf_P(PSTR(PRIPSTR"\t ="),name);
  while (qty--)
    printf_P(PSTR(" 0x%02x"),RF24_read_register(reg++));
  printf_P(PSTR("\r\n"));
}

/****************************************************************************/

void RF24_print_address_register(const char* name, uint8_t reg, uint8_t qty)
{
  printf_P(PSTR(PRIPSTR"\t ="),name);
  while (qty--)
  {
    uint8_t buffer[5];
	uint8_t* bufptr;

    RF24_read_register_bytes(reg++, buffer, addr_width);
    printf_P(PSTR(" 0x"));
    bufptr = buffer + addr_width;
    while( --bufptr >= buffer )
      printf_P(PSTR("%02x"),*bufptr);
  }
  printf_P(PSTR("\r\n"));
}

static const char rf24_datarate_e_str_0[] PROGMEM = "1MBPS";
static const char rf24_datarate_e_str_1[] PROGMEM = "2MBPS";
static const char rf24_datarate_e_str_2[] PROGMEM = "250KBPS";
static const char * const rf24_datarate_e_str_P[] PROGMEM = {
  rf24_datarate_e_str_0,
  rf24_datarate_e_str_1,
  rf24_datarate_e_str_2,
};
static const char rf24_model_e_str_0[] PROGMEM = "nRF24L01";
static const char rf24_model_e_str_1[] PROGMEM = "nRF24L01+";
static const char * const rf24_model_e_str_P[] PROGMEM = {
  rf24_model_e_str_0,
  rf24_model_e_str_1,
};
static const char rf24_crclength_e_str_0[] PROGMEM = "Disabled";
static const char rf24_crclength_e_str_1[] PROGMEM = "8 bits";
static const char rf24_crclength_e_str_2[] PROGMEM = "16 bits" ;
static const char * const rf24_crclength_e_str_P[] PROGMEM = {
  rf24_crclength_e_str_0,
  rf24_crclength_e_str_1,
  rf24_crclength_e_str_2,
};
static const char rf24_pa_dbm_e_str_0[] PROGMEM = "PA_MIN";
static const char rf24_pa_dbm_e_str_1[] PROGMEM = "PA_LOW";
static const char rf24_pa_dbm_e_str_2[] PROGMEM = "PA_HIGH";
static const char rf24_pa_dbm_e_str_3[] PROGMEM = "PA_MAX";
static const char * const rf24_pa_dbm_e_str_P[] PROGMEM = {
  rf24_pa_dbm_e_str_0,
  rf24_pa_dbm_e_str_1,
  rf24_pa_dbm_e_str_2,
  rf24_pa_dbm_e_str_3,
};

void RF24_printDetails(void)
{

  printf("\n================ NRF Configuration ================\n");
  RF24_print_status(RF24_getStatus());
  RF24_print_address_register(PSTR("RX_ADDR_P0-1"),RX_ADDR_P0,2);
  RF24_print_byte_register(PSTR("RX_ADDR_P2-5"),RX_ADDR_P2,4);
  RF24_print_address_register(PSTR("TX_ADDR\t"),TX_ADDR, 1);

  RF24_print_byte_register(PSTR("RX_PW_P0-6"),RX_PW_P0,6);
  RF24_print_byte_register(PSTR("EN_AA\t"),EN_AA, 1);
  RF24_print_byte_register(PSTR("EN_RXADDR"),EN_RXADDR, 1);
  RF24_print_byte_register(PSTR("RF_CH\t"),RF_CH, 1);
  RF24_print_byte_register(PSTR("RF_SETUP"),RF_SETUP, 1);
  RF24_print_byte_register(PSTR("CONFIG\t"),CONFIG, 1);
  RF24_print_byte_register(PSTR("DYNPD/FEATURE"),DYNPD, 2);

  printf_P(PSTR("Data Rate\t = %s\r\n"),pgm_read_word(&rf24_datarate_e_str_P[RF24_getDataRate()]));
  printf_P(PSTR("Model\t\t = %s\r\n"),pgm_read_word(&rf24_model_e_str_P[RF24_isPVariant()]));
  printf_P(PSTR("CRC Length\t = %s\r\n"),pgm_read_word(&rf24_crclength_e_str_P[RF24_getCRCLength()]));
  printf_P(PSTR("PA Power\t = %s\r\n"),  pgm_read_word(&rf24_pa_dbm_e_str_P[RF24_getPALevel()]));
}

#endif

/****************************************************************************/

void RF24_setChannel(uint8_t channel)
{
  RF24_write_register(RF_CH, channel & 0x7F);
}

uint8_t RF24_getChannel()
{
  return RF24_read_register(RF_CH);
}

/****************************************************************************/

void RF24_Init()
{
  p_variant = 0;
  addr_width = 5;
  dynamic_payload_enabled = 0;
  ack_payload_enabled = 0;
  pipe0_enabled = 0;
  pipe0_payload_size = 16;
  memset(pipe0_reading_address, 0x0F, addr_width);
	
 
  // Must allow the radio time to settle else configuration bits will not necessarily stick.
  // This is actually only required following power up but some settling time also appears to
  // be required after resets too. For full coverage, we'll always assume the worst.
  // Enabling 16b CRC is by far the most obvious case if the wrong timing is used - or skipped.
  // Technically we require 4.5ms + 14us as a worst case. We'll just call it 5ms for good measure.
  // WARNING: Delay is based on P-variant whereby non-P *may* require different timing.

  RF24_ce(LOW);
  delay( 5 ) ;

  // Reset CONFIG - Power Down, enable 16-bit CRC.
  RF24_config = 0x0C;
  RF24_write_register( CONFIG, RF24_config ) ; 
  RF24_write_register( CONFIG, RF24_config ) ;  // We need to send command second time in case the clock was not correct
  while(RF24_read_register(CONFIG) != RF24_config)  
	RF24_write_register( CONFIG, RF24_config ) ;
  
  // Set 1500uS (minimum for 32B payload in ESB@250KBPS) timeouts, to make testing a little easier
  // WARNING: If this is ever lowered, either 250KBS mode with AA is broken or maximum packet
  // sizes must never be used. See documentation for a more complete explanation.
  RF24_setRetries(5,15);

  // Reset value is MAX
  RF24_setPALevel( RF24_PA_MAX ) ;

  // check for connected module and if this is a p nRF24l01 variant
  //
  if( RF24_setDataRate( RF24_250KBPS ) )
  {
    p_variant = TRUE ;
  }
  
  // Then set the data rate to the most common speed supported by all
  // hardware.
  RF24_setDataRate( RF24_1MBPS ) ;

  
  // Set address width to 5
  RF24_setAddressWidth(5);
  
  // Disable dynamic payloads, to match dynamic_payload_enabled setting - Reset value is 0
  RF24_toggle_features();
  RF24_write_register(FEATURE,0 );
  RF24_write_register(DYNPD,0);
  RF24_write_register(EN_AA,0);
  RF24_write_register(EN_RXADDR, 0);

  // Set up default configuration.  Callers can always change it later.
  // This channel should be universally safe and not bleed over into adjacent
  // spectrum.
  RF24_setChannel(76);  
  
  // Reset current status
  // Notice: reset and flush is the last thing we do
  RF24_write_register(NRF_STATUS, _BV(RX_DR) | _BV(TX_DS) | _BV(MAX_RT) );

  // Flush buffers
  RF24_flushRx();
  RF24_flushTx();

  RF24_powerUp(); //Power up 
  
  // Enable RTX so radio will remain in standby I mode 
  //  ( 130us max to transition to RX or TX instead of 1500us from powerUp )
  // PRX should use only 22uA of power
}

/****************************************************************************/
static const uint8_t child_pipe[] PROGMEM =
{
  RX_ADDR_P0, RX_ADDR_P1, RX_ADDR_P2, RX_ADDR_P3, RX_ADDR_P4, RX_ADDR_P5
};

static const uint8_t child_pipe_enable[] PROGMEM =
{
  ERX_P0, ERX_P1, ERX_P2, ERX_P3, ERX_P4, ERX_P5
};

static const uint8_t child_payload_size[] PROGMEM =
{
  RX_PW_P0, RX_PW_P1, RX_PW_P2, RX_PW_P3, RX_PW_P4, RX_PW_P5
};

/****************************************************************************/
void RF24_startListening()
{
  RF24_config |= _BV(PRIM_RX);
  RF24_write_register(CONFIG, RF24_config );
  RF24_write_register(NRF_STATUS, _BV(RX_DR) | _BV(TX_DS) | _BV(MAX_RT) );

  // Restore the pipe0 adddress, and payload size, if enabled
  if (pipe0_enabled){
     RF24_write_register_bytes(RX_ADDR_P0, pipe0_reading_address, addr_width);
	 RF24_write_register(RX_PW_P0, pipe0_payload_size);
     RF24_write_register(EN_RXADDR, RF24_read_register(EN_RXADDR) | _BV(ERX_P0));	  
  }	
  // Flush buffers
  RF24_flushRx();
  RF24_flushTx();
  RF24_ce(HIGH);
}

void RF24_startListeningFast()
{
  RF24_config |= _BV(PRIM_RX);	
  RF24_write_register(CONFIG, RF24_config );
  RF24_flushRx();
  RF24_ce(HIGH);
}

void RF24_stopListening(void)
{  
  RF24_ce(LOW);
  RF24_flushTx();
  RF24_flushRx();
  RF24_write_register(NRF_STATUS, _BV(RX_DR) | _BV(TX_DS) | _BV(MAX_RT) );	
  RF24_config &= ~_BV(PRIM_RX);
  RF24_write_register(CONFIG, RF24_config );	
}

void RF24_stopListeningFast(void)
{  
  RF24_ce(LOW);
  RF24_flushRx();
  RF24_config &= ~_BV(PRIM_RX);
  RF24_write_register(CONFIG, RF24_config );	
}

/****************************************************************************/

void RF24_powerDown(void)
{
  RF24_ce(LOW); // Guarantee CE is low on powerDown
  RF24_config &= ~_BV(PWR_UP);	
  RF24_write_register(CONFIG, RF24_config );
}

/****************************************************************************/

//Power up now. Radio will not power down unless instructed by MCU for config changes etc.
void RF24_powerUp(void)
{
   // if not powered up then power up and wait for the radio to initialize
   if (!(RF24_config & _BV(PWR_UP))){
	  RF24_config |=  _BV(PWR_UP);
      RF24_write_register(CONFIG, RF24_config);
      // For nRF24L01+ to go from power down mode to TX or RX mode it must first pass through stand-by mode.
	  // There must be a delay of Tpd2stby (see Table 16.) after the nRF24L01+ leaves power down mode before
	  // the CE is set high. - Tpd2stby can be up to 5ms per the 1.0 datasheet
      delay(5);
   }
}

/******************************************************************/

/****************************************************************************/

//For general use, the interrupt flags are not important to clear
int RF24_WaitAndWrite( const void* buf, uint8_t len, uint32_t timeout )
{
	uint8_t status;
	//Block until the FIFO is NOT full.
	//Keep track of the MAX retries and set auto-retry if seeing failures
	//This way the FIFO will fill up and allow blocking until packets go through
	//The radio will auto-clear everything in the FIFO as long as CE remains high
	uint32_t timer = millis();							  	//Get the time that the payload transmission started
	while( ( status = RF24_getStatus() ) & _BV(TX_FULL) ) {	//Blocking only if FIFO is full. This will loop and block until TX is successful or timeout
		if( (status & _BV(MAX_RT)) ||    					//If MAX Retries have been reached
		     (((millis() - timer) > timeout))) return 0;		//If this payload has exceeded the user-defined timeout, exit and return 0
  	}
  	//Start Writing
	RF24_write_payload(buf, len);							//Write the payload if a buffer is clear
	return 1;												//Return 1 to indicate successful transmission
}

/****************************************************************************/

int RF24_writeAck( const void* buf, uint8_t len, const int multicast )
{
	//Send ONLY when Tx FIFO is empty - we need this to keep predictable timing.
	if( !(RF24_read_register(FIFO_STATUS) & _BV(TX_EMPTY)) ){
	  if( RF24_getStatus() & _BV(MAX_RT)) {
	    RF24_flushTx();		  
		RF24_write_register(NRF_STATUS,_BV(MAX_RT) );	  //Clear max retry flag
	  }
	  return 0;										  //Return error  	
	}
	//Start Writing	
	RF24_startWrite(buf,len,multicast);	
	return 1;
}

int RF24_write( const void* buf, uint8_t len ){
	return RF24_writeAck(buf, len, 0);
}

/****************************************************************************/

//Per the documentation, we want to set PTX Mode when not listening. Then all we do is write data and set CE high
//In this mode, if we can keep the FIFO buffers loaded, packets will transmit immediately (no 130us delay)
//Otherwise we enter Standby-II mode, which is still faster than standby mode
//Also, we remove the need to keep writing the config register over and over and delaying for 150 us each time if sending a stream of data

void RF24_startWrite( const void* buf, uint8_t len, const int multicast){ 
	if(multicast)
		RF24_write_payload_no_ack(buf, len); 		
	else
		RF24_write_payload(buf, len);
}

/****************************************************************************/

int RF24_rxFifoFull(){
	return RF24_read_register(FIFO_STATUS) & _BV(RX_FULL);
}
/****************************************************************************/

int RF24_txStandBy(){

	while( !(RF24_read_register(FIFO_STATUS) & _BV(TX_EMPTY)) ){
		if( RF24_getStatus() & _BV(MAX_RT)){
			RF24_write_register(NRF_STATUS,_BV(MAX_RT) );
			RF24_flushTx();    //Non blocking, flush the data
			return 0;
		}
	}
	return 1;
}

/****************************************************************************/

int RF24_txStandByAndWait(uint32_t timeout){

	uint32_t start = millis();

	while( ! (RF24_read_register(FIFO_STATUS) & _BV(TX_EMPTY)) ){
		if( RF24_getStatus() & _BV(MAX_RT)){
			RF24_write_register(NRF_STATUS,_BV(MAX_RT) );										  //Set re-transmit
			if(millis() - start >= timeout){
				RF24_flushTx(); 
				return 0;
			}
		}
	}	
	return 1;
}

/****************************************************************************/

void RF24_maskIRQ(int tx, int fail, int rx){

	RF24_config =  ( RF24_config  & 
		~(1 << MASK_MAX_RT | 1 << MASK_TX_DS | 1 << MASK_RX_DR)) | 
			(fail << MASK_MAX_RT | tx << MASK_TX_DS | rx << MASK_RX_DR) ;
	RF24_write_register(CONFIG, RF24_config);
}

/****************************************************************************/

uint8_t RF24_getDynamicPayloadSize(void)
{
  uint8_t result;
  result = NRF24L01_ReadByte( R_RX_PL_WID );
  if(result > MAX_PAYLOAD_SIZE) { 
	RF24_flushRx(); 
    return 0; 
  }
  return result;
}

/****************************************************************************/

int RF24_rxAvailableAny(void)
{
  return RF24_rxAvailable(NULL);
}

/****************************************************************************/

int RF24_rxAvailable(uint8_t *pipe)
{
  if (!( RF24_read_register(FIFO_STATUS) & _BV(RX_EMPTY) )){
    // If the caller wants the pipe number, include that
	int  pipe_num = ( RF24_getStatus() >> RX_P_NO ) & 0x07;
    if ( pipe ){
      *pipe = pipe_num; 
  	}
  	return dynamic_payload_enabled ? 
		RF24_getDynamicPayloadSize() : 
		RF24_read_register(child_payload_size[pipe_num]);
  }
  return 0;
}

/****************************************************************************/

void RF24_read( void* buf, uint8_t len ){

  // Fetch the payload
  RF24_read_payload( buf, len );
  //Clear the two possible interrupt flags with one command
  RF24_write_register(NRF_STATUS,_BV(RX_DR) | _BV(MAX_RT) | _BV(TX_DS) );
}

/****************************************************************************/

void RF24_whatHappened(int *tx_ok,int *tx_fail,int  *rx_ready)
{
  // Read the status & reset the status in one easy call
  uint8_t status = RF24_write_register(NRF_STATUS,_BV(RX_DR) | _BV(TX_DS) | _BV(MAX_RT) );

  // Report to the user what happened
	*tx_ok = status & _BV(TX_DS) ? 1 : 0;
	*tx_fail = status & _BV(MAX_RT) ? 1 : 0;
	*rx_ready = status & _BV(RX_DR) ? 1 : 0;
}

/****************************************************************************/

/****************************************************************************/
void RF24_openWritingPipe(uint8_t *address)
{
  // The NRF24L01(+) expects address LSB first.
  RF24_write_register_bytes(TX_ADDR, address, addr_width);
  // At the same time open pipe0 to receive automatic ACKs
  RF24_write_register_bytes(RX_ADDR_P0,address, addr_width);
  RF24_write_register(RX_PW_P0, pipe0_payload_size);
  RF24_write_register(EN_RXADDR, RF24_read_register(EN_RXADDR) | _BV(ERX_P0));		
}

/****************************************************************************/

/****************************************************************************/
void RF24_setAddressWidth(uint8_t a_width){

	RF24_write_register(SETUP_AW, a_width - 2);
	addr_width = a_width;
}

/****************************************************************************/

void RF24_openReadingPipe(uint8_t pipe, uint8_t *address, uint8_t pipe_payload_size)
{
  // If this is pipe 0, cache the address and payload_size.  This is needed because
  // openWritingPipe() will overwrite the pipe 0 address, so
  // startListening() will have to restore it.
  if (pipe == 0){
    memcpy(pipe0_reading_address, address, addr_width);
	pipe0_enabled = TRUE;
	pipe0_payload_size = pipe_payload_size;
  }

	// For pipes 2-5, only write the LSB, for 0 and 1 - full address
	if ( pipe < 2 ){
	  RF24_write_register_bytes(pgm_read_byte(&child_pipe[pipe]), address, addr_width);
	}else{
	  RF24_write_register_bytes(pgm_read_byte(&child_pipe[pipe]), address, 1);
	}
	RF24_write_register(pgm_read_byte(&child_payload_size[pipe]), pipe_payload_size);

	// Note it would be more efficient to set all of the bits for all open
	// pipes at once.  However, I thought it would make the calling code
	// more simple to do it this way.
	RF24_write_register(EN_RXADDR, RF24_read_register(EN_RXADDR) | _BV(pgm_read_byte(&child_pipe_enable[pipe])));
}

/****************************************************************************/

void RF24_closeReadingPipe( uint8_t pipe )
{
  if(pipe == 0)
  {
	pipe0_enabled = FALSE;
  }
  RF24_write_register(EN_RXADDR,RF24_read_register(EN_RXADDR) & ~_BV(pgm_read_byte(&child_pipe_enable[pipe])));
}

/****************************************************************************/

void RF24_toggle_features(void)
{
	NRF24L01_WriteByte(ACTIVATE, 0x73);
}

/****************************************************************************/

void RF24_setDynamicPayload(int enable)
{
  // Enable/disable dynamic payload throughout the system
  if(enable)  
	RF24_write_register(FEATURE, RF24_read_register(FEATURE) | _BV(EN_DPL) );
  else
	RF24_write_register(FEATURE, RF24_read_register(FEATURE) & ~_BV(EN_DPL) );
  
  // Enable dynamic payload on all pipes
  if(enable)
	RF24_write_register(DYNPD, _BV(DPL_P5) | _BV(DPL_P4) | _BV(DPL_P3) | _BV(DPL_P2) | _BV(DPL_P1) | _BV(DPL_P0));
  else
	RF24_write_register(DYNPD, 0);
	  
  dynamic_payload_enabled = enable;
}

/****************************************************************************/

void RF24_setAckPayload(int enable)
{
  //
  // enable ack payload and dynamic payload features
  //
  if(enable)  
	RF24_write_register(FEATURE, RF24_read_register(FEATURE) | (_BV(EN_ACK_PAY) | _BV(EN_DPL)) );
  else
	RF24_write_register(FEATURE, RF24_read_register(FEATURE) & ~(_BV(EN_ACK_PAY) | _BV(EN_DPL)) );		
  //
  // Enable dynamic payload on all pipes
  //
  if(enable)
	RF24_write_register(DYNPD, _BV(DPL_P5) | _BV(DPL_P4) | _BV(DPL_P3) | _BV(DPL_P2) | _BV(DPL_P1) | _BV(DPL_P0));
  else
	RF24_write_register(DYNPD, 0);

  ack_payload_enabled = enable;
  dynamic_payload_enabled = enable;
}

/****************************************************************************/

void RF24_setDynamicAck(int enable){
  //
  // enable/disable  dynamic ack features
  //
  if(enable)
	RF24_write_register(FEATURE,RF24_read_register(FEATURE) | _BV(EN_DYN_ACK) );
  else
	RF24_write_register(FEATURE,RF24_read_register(FEATURE) & ~_BV(EN_DYN_ACK) );	  
}

/****************************************************************************/

void RF24_writeAckPayload(uint8_t pipe, void *buf, uint8_t len)
{
	if(ack_payload_enabled)
		NRF24L01_Write(W_ACK_PAYLOAD | ( pipe & 0x07 ), buf, len);
}

/****************************************************************************/

int RF24_isAckPayloadAvailable(void)
{
  return !(RF24_read_register(FIFO_STATUS) & _BV(RX_EMPTY));
}

/****************************************************************************/

int RF24_isPVariant(void)
{
  return p_variant ;
}

/****************************************************************************/

void RF24_setAutoAckAll(int enable)
{
  RF24_write_register(EN_AA,  enable ? 0x3F : 0);
}

/****************************************************************************/

void RF24_setAutoAck(uint8_t pipe, int enable )
{
    uint8_t en_aa = RF24_read_register( EN_AA ) ;
    if( enable ){
      en_aa |= _BV(pipe) ;
    } else {
      en_aa &= ~_BV(pipe) ;
    }
    RF24_write_register( EN_AA, en_aa ) ;
}

/****************************************************************************/

int RF24RF24_testCarrier(void)
{
	return ( p_variant ? RF24_read_register(RPD) & 1 : RF24_read_register(CD) & 1 );
}

/****************************************************************************/

void RF24_setPALevel(uint8_t level)
{
  uint8_t setup = RF24_read_register(RF_SETUP) & 0xF8;

  if(level > 3){  						// If invalid level, go to max PA
	  level = (RF24_PA_MAX << 1) + 1;		// +1 to support the SI24R1 chip extra bit
  }else{
	  level = (level << 1) + 1;	 		// Else set level as requested
  }
  RF24_write_register( RF_SETUP, setup |= level ) ;	// Write it to the chip
}

/****************************************************************************/

uint8_t RF24_getPALevel(void)
{
  return (RF24_read_register(RF_SETUP) & (_BV(RF_PWR_LOW) | _BV(RF_PWR_HIGH))) >> 1 ;
}

/****************************************************************************/

int RF24_setDataRate(rf24_datarate_e speed)
{
  int result = FALSE;
  uint8_t setup = RF24_read_register(RF_SETUP) ;

  // HIGH and LOW '00' is 1Mbs - our default
  setup &= ~(_BV(RF_DR_LOW) | _BV(RF_DR_HIGH)) ;
  txRxDelay=250;
  if( speed == RF24_250KBPS )
  {
    // Must set the RF_DR_LOW to 1; RF_DR_HIGH (used to be RF_DR) is already 0
    // Making it '10'.
    setup |= _BV( RF_DR_LOW ) ;
    txRxDelay=450;
  } else {
    // Set 2Mbs, RF_DR (RF_DR_HIGH) is set 1
    // Making it '01'
    if ( speed == RF24_2MBPS )
    {
      setup |= _BV(RF_DR_HIGH);
      txRxDelay=190;
    }
  }
  RF24_write_register(RF_SETUP,setup);

  // Verify our result
  if ( RF24_read_register(RF_SETUP) == setup )
  {
    result = TRUE;
  }
  return result;
}

/****************************************************************************/

rf24_datarate_e RF24_getDataRate( void )
{
  rf24_datarate_e result ;
  uint8_t dr = RF24_read_register(RF_SETUP) & (_BV(RF_DR_LOW) | _BV(RF_DR_HIGH));

  // Order matters in our case below
  if ( dr == _BV(RF_DR_LOW) )
  {
    // '10' = 250KBPS
    result = RF24_250KBPS ;
  }else if ( dr == _BV(RF_DR_HIGH) )
  {
    // '01' = 2MBPS
    result = RF24_2MBPS ;
  }else
  {
    // '00' = 1MBPS
    result = RF24_1MBPS ;
  }
  return result ;
}

/****************************************************************************/

void RF24_setCRCLength(rf24_crclength_e length)
{
  RF24_config = RF24_read_register(CONFIG) & ~( _BV(CRCO) | _BV(EN_CRC)) ;

  if ( length == RF24_CRC_DISABLED )
  {
    // Do nothing, we turned it off above.
  }else if ( length == RF24_CRC_8 )
  {
    RF24_config |= _BV(EN_CRC);
  }else{
    RF24_config |= _BV(EN_CRC);
    RF24_config |= _BV( CRCO );
  }
  RF24_write_register( CONFIG, RF24_config) ;
}

/****************************************************************************/

rf24_crclength_e RF24_getCRCLength(void)
{
  rf24_crclength_e result = RF24_CRC_DISABLED;
  
  uint8_t config = RF24_read_register(CONFIG) & ( _BV(CRCO) | _BV(EN_CRC)) ;
  uint8_t AA = RF24_read_register(EN_AA);
  
  if ( config & _BV(EN_CRC ) || AA)
  {
    if ( config & _BV(CRCO) )
      result = RF24_CRC_16;
    else
      result = RF24_CRC_8;
  }

  return result;
}

/****************************************************************************/
void RF24_setRetries(uint8_t delay, uint8_t count)
{
	RF24_write_register(SETUP_RETR,(delay&0xf)<<ARD | (count&0xf)<<ARC);
}
