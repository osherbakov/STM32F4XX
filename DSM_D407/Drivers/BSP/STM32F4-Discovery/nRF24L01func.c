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

#define delay	HAL_Delay
#define millis	HAL_GetTick

/****************************************************************************/

static  int 	p_variant; 				/* False for RF24L01 and true for RF24L01P */
static  int 	dynamic_payload_enabled; /**< Whether dynamic payloads are enabled. */
static  int   	ack_payload_enabled;


static  uint8_t pipe0_reading_address[5]; /**< Last address set on pipe 0 for reading. */
static  uint8_t pipe0_payload_size;
static  uint8_t pipe0_enabled;
  
static  uint8_t addr_width; 			/**< The address width to use - 3,4 or 5 bytes. */
static  uint8_t RF24_config;

/****************************************************************************/
uint8_t RF24_read(uint8_t reg, uint8_t* buf, uint8_t len)
{
  return NRF24L01_Read(R_REGISTER | ( REGISTER_MASK & reg ), buf, len);
}

/****************************************************************************/

uint8_t RF24_read_register(uint8_t reg)
{
  return NRF24L01_ReadByte(R_REGISTER | ( REGISTER_MASK & reg ));
}

/****************************************************************************/

uint8_t RF24_write(uint8_t reg, uint8_t* buf, uint8_t len)
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
  return NRF24L01_StartWrite(W_TX_PAYLOAD, (uint8_t *)buf, data_len);
}

uint8_t RF24_write_payload_no_ack(const void* buf, uint8_t data_len)
{
  return NRF24L01_StartWrite(W_TX_PAYLOAD_NO_ACK, (uint8_t *)buf, data_len);
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
  RF24_write_register( CONFIG, RF24_config ) ;  // We need to send command second time in case the clock was not correct
  while(RF24_read_register(CONFIG) != RF24_config)  
	RF24_write_register( CONFIG, RF24_config ) ;
  
  // Set 1500uS (minimum for 32B payload in ESB@250KBPS) timeouts, to make testing a little easier
  // WARNING: If this is ever lowered, either 250KBS mode with AA is broken or maximum packet
  // sizes must never be used. See documentation for a more complete explanation.
  RF24_setRetries(5,15);

  // Reset PA value is MAX
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
  RF24_setAddressWidth(addr_width);
  
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
     RF24_write(RX_ADDR_P0, pipe0_reading_address, addr_width);
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

/****************************************************************************/
void RF24_writeFast( const void* buf, uint8_t len)
{
	RF24_write_payload(buf, len); 
}

int RF24_writeAck( const void* buf, uint8_t len)
{
	//Send ONLY when Tx FIFO is empty - we need this to keep predictable timing.
	if( !(RF24_read_register(FIFO_STATUS) & _BV(TX_EMPTY)) ){
	  if( RF24_getStatus() & _BV(MAX_RT)) {
	    RF24_flushTx();		  
		RF24_write_register(NRF_STATUS,_BV(MAX_RT) );	  //Clear max retry flag
	  }
	  return 0;										  //Return error  	
	}
	RF24_write_payload(buf, len); 
	return 1;
}

int RF24_writeNoAck( const void* buf, uint8_t len ){
	if( !(RF24_read_register(FIFO_STATUS) & _BV(TX_EMPTY)) ){
	  if( RF24_getStatus() & _BV(MAX_RT)) {
	    RF24_flushTx();		  
		RF24_write_register(NRF_STATUS,_BV(MAX_RT) );	  //Clear max retry flag
	  }
	  return 0;										  //Return error  	
	}
	RF24_write_payload_no_ack(buf, len); 
	return 1;	
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

int RF24_txWait(uint32_t timeout){

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
  RF24_write(TX_ADDR, address, addr_width);
  // At the same time open pipe0 to receive automatic ACKs
  RF24_write(RX_ADDR_P0,address, addr_width);
  RF24_write_register(RX_PW_P0, pipe0_payload_size);
  RF24_write_register(EN_RXADDR, RF24_read_register(EN_RXADDR) | _BV(ERX_P0));		
}

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
	  RF24_write(pgm_read_byte(&child_pipe[pipe]), address, addr_width);
	}else{
	  RF24_write(pgm_read_byte(&child_pipe[pipe]), address, 1);
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

  if( speed == RF24_250KBPS )
  {
    // Must set the RF_DR_LOW to 1; RF_DR_HIGH (used to be RF_DR) is already 0
    // Making it '10'.
    setup |= _BV( RF_DR_LOW ) ;

  } else {
    // Set 2Mbs, RF_DR (RF_DR_HIGH) is set 1
    // Making it '01'
    if ( speed == RF24_2MBPS )
    {
      setup |= _BV(RF_DR_HIGH);
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

static uint8_t	 _Tx[32] = {1,2,3,4,5,6,7,8,9,10,0x55, 0xF9, 0xAF, 0x12, 0x55, 0xAA};
static uint8_t	 _Rx[32];
static uint8_t	 TxAddress[] = "1Node";
static uint8_t	 TxChannel = 0;
static int		 RxMode = 1;

void StartRF24(void)
{
	RF24_Init();
	RF24_setAddressWidth(5);
	RF24_setDynamicPayload(0);
	RF24_setAckPayload(0);
	RF24_setAutoAckAll(0);
	RF24_setDataRate(RF24_250KBPS);
	RF24_openReadingPipe(0, TxAddress, 16);	
	RF24_openWritingPipe(TxAddress);		

	if (RxMode) 
		RF24_startListening();
	else 
		RF24_stopListening();		
			
}

void ProcessRF24(void)
{
	if(RxMode == 0){
		//			RF24_stopListeningFast();		
		// RF24_setChannel(TxChannel++);
		RF24_writeFast(_Tx, 16);
		//			RxMode = 1;
	}else{
		//			RF24_startListeningFast();
		//			RxMode = 0;
	}		
}
