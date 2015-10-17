/*
 Copyright (C) 2011 J. Coliz <maniacbug@ymail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

#include "nRF24L01.h"
#include "nRF24L01conf.h"
#include "nRF24L01regs.h"

#define RF24_ce(a) ((a) == LOW) ? NRF24_CE_LOW() : NRF24_CE_HIGH()

/****************************************************************************/


  int p_variant; 			/* False for RF24L01 and true for RF24L01P */
  int dynamic_payloads_enabled; /**< Whether dynamic payloads are enabled. */
  int failureDetected; 

  uint8_t payload_size; 	/**< Fixed size of payloads */
  uint8_t pipe0_reading_address[5]; /**< Last address set on pipe 0 for reading. */
  uint8_t addr_width; 		/**< The address width to use - 3,4 or 5 bytes. */
  uint32_t txRxDelay; 		/**< Var for adjusting delays depending on datarate */

/****************************************************************************/
uint8_t RF24_read_register(uint8_t reg, uint8_t* buf, uint8_t len)
{
  return NRF24L01_Read(R_REGISTER | ( REGISTER_MASK & reg ), buf, len);
}

/****************************************************************************/

uint8_t RF24_read_register(uint8_t reg)
{
  return NRF24L01_ReadByte(R_REGISTER | ( REGISTER_MASK & reg ));
}

/****************************************************************************/

uint8_t RF24_write_register(uint8_t reg, const uint8_t* buf, uint8_t len)
{
  return NRF24L01_Write(W_REGISTER | ( REGISTER_MASK & reg ), buf, len);
}

/****************************************************************************/

uint8_t RF24_write_register(uint8_t reg, uint8_t value)
{
  return NRF24L01_WriteByte(W_REGISTER | ( REGISTER_MASK & reg ), value);
}

/****************************************************************************/

uint8_t RF24_write_payload(const void* buf, uint8_t data_len, const uint8_t writeType)
{
  return NRF24L01_Write(writeType, buf, data_len);
}

/****************************************************************************/

uint8_t RF24_read_payload(void* buf, uint8_t data_len)
{
  return NRF24L01_Read(R_RX_PAYLOAD, buf, data_len);
}

/****************************************************************************/

uint8_t RF24_flushRx(void)
{
  return NRF24L01_TouchByte(FLUSH_RX);
}

/****************************************************************************/

uint8_t RF24_flushTx(void)
{
  return NRF24L01_TouchByte(FLUSH_TX);
}

/****************************************************************************/

uint8_t RF24_get_status(void)
{
  return NRF24L01_TouchByte(NOP);
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
           ((status >> RX_P_NO) & 0b111),
           (status & _BV(TX_FULL))?1:0
          );
}

/****************************************************************************/

void RF24_print_observe_tx(uint8_t value)
{
  printf_P(PSTR("OBSERVE_TX=%02x: POLS_CNT=%x ARC_CNT=%x\r\n"),
           value,
           (value >> PLOS_CNT) & 0b1111,
           (value >> ARC_CNT) & 0b1111
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

    RF24_read_register(reg++, buffer, addr_width);
    printf_P(PSTR(" 0x"));
    bufptr = buffer + addr_width;
    while( --bufptr >= buffer )
      printf_P(PSTR("%02x"),*bufptr);
  }
  printf_P(PSTR("\r\n"));
}
#endif
/****************************************************************************/

RF24_Init()
{
  p_variant = 0;
  payload_size = 32;
  dynamic_payloads_enabled = 0;
  addr_width = 5;
  pipe0_reading_address[0] = 0;
}

/****************************************************************************/

void RF24_setChannel(uint8_t channel)
{
  RF24_write_register(RF_CH, rf24_min(channel,127));
}

uint8_t RF24_getChannel()
{
  return RF24_read_register(RF_CH);
}
/****************************************************************************/

void RF24_setPayloadSize(uint8_t size)
{
  payload_size = rf24_min(size,32);
}

/****************************************************************************/

uint8_t RF24_getPayloadSize(void)
{
  return payload_size;
}

/****************************************************************************/

#if !defined (MINIMAL)

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

void RF24::printDetails(void)
{

  printf("\n================ NRF Configuration ================\n");
  print_status(get_status());
  print_address_register(PSTR("RX_ADDR_P0-1"),RX_ADDR_P0,2);
  print_byte_register(PSTR("RX_ADDR_P2-5"),RX_ADDR_P2,4);
  print_address_register(PSTR("TX_ADDR\t"),TX_ADDR);

  print_byte_register(PSTR("RX_PW_P0-6"),RX_PW_P0,6);
  print_byte_register(PSTR("EN_AA\t"),EN_AA);
  print_byte_register(PSTR("EN_RXADDR"),EN_RXADDR);
  print_byte_register(PSTR("RF_CH\t"),RF_CH);
  print_byte_register(PSTR("RF_SETUP"),RF_SETUP);
  print_byte_register(PSTR("CONFIG\t"),CONFIG);
  print_byte_register(PSTR("DYNPD/FEATURE"),DYNPD,2);

  printf_P(PSTR("Data Rate\t = %s\r\n"),pgm_read_word(&rf24_datarate_e_str_P[getDataRate()]));
  printf_P(PSTR("Model\t\t = %s\r\n"),pgm_read_word(&rf24_model_e_str_P[isPVariant()]));
  printf_P(PSTR("CRC Length\t = %s\r\n"),pgm_read_word(&rf24_crclength_e_str_P[getCRCLength()]));
  printf_P(PSTR("PA Power\t = %s\r\n"),  pgm_read_word(&rf24_pa_dbm_e_str_P[getPALevel()]));
}

#endif
/****************************************************************************/

int RF24_begin(void)
{
  uint8_t setup=0;
 
  // Must allow the radio time to settle else configuration bits will not necessarily stick.
  // This is actually only required following power up but some settling time also appears to
  // be required after resets too. For full coverage, we'll always assume the worst.
  // Enabling 16b CRC is by far the most obvious case if the wrong timing is used - or skipped.
  // Technically we require 4.5ms + 14us as a worst case. We'll just call it 5ms for good measure.
  // WARNING: Delay is based on P-variant whereby non-P *may* require different timing.
  delay( 5 ) ;

  // Reset CONFIG and enable 16-bit CRC.
  RF24_write_register( CONFIG, 0b00001100 ) ;

  // Set 1500uS (minimum for 32B payload in ESB@250KBPS) timeouts, to make testing a little easier
  // WARNING: If this is ever lowered, either 250KBS mode with AA is broken or maximum packet
  // sizes must never be used. See documentation for a more complete explanation.
  RF24_setRetries(5,15);

  // Reset value is MAX
  setPALevel( RF24_PA_MAX ) ;

  // check for connected module and if this is a p nRF24l01 variant
  //
  if( RF24_setDataRate( RF24_250KBPS ) )
  {
    p_variant = true ;
  }
  /*setup = read_register(RF_SETUP);
  if( setup == 0b00001110 )     // register default for nRF24L01P
  {
    p_variant = true ;
  }*/
  
  // Then set the data rate to the slowest (and most reliable) speed supported by all
  // hardware.
  RF24_setDataRate( RF24_1MBPS ) ;

  // Initialize CRC and request 2-byte (16bit) CRC
  setCRCLength( RF24_CRC_16 ) ;

  // Disable dynamic payloads, to match dynamic_payloads_enabled setting - Reset value is 0
  RF24_toggle_features();
  RF24_write_register(FEATURE,0 );
  RF24_write_register(DYNPD,0);

  // Reset current status
  // Notice reset and flush is the last thing we do
  RF24_write_register(NRF_STATUS,_BV(RX_DR) | _BV(TX_DS) | _BV(MAX_RT) );

  // Set up default configuration.  Callers can always change it later.
  // This channel should be universally safe and not bleed over into adjacent
  // spectrum.
  RF24_setChannel(76);

  // Flush buffers
  RF24_flushRx();
  RF24_flushTx();

  RF24_powerUp(); //Power up by default when begin() is called

  // Enable PTX, do not write CE high so radio will remain in standby I mode ( 130us max to transition to RX or TX instead of 1500us from powerUp )
  // PTX should use only 22uA of power
  RF24_write_register(CONFIG, ( read_register(CONFIG) ) & ~_BV(PRIM_RX) );

  // if setup is 0 or ff then there was no response from module
  return ( setup != 0 && setup != 0xff );
}

/****************************************************************************/

void RF24_startListening(void)
{
  RF24_powerUp();
  RF24_write_register(CONFIG, read_register(CONFIG) | _BV(PRIM_RX));
  RF24_write_register(NRF_STATUS, _BV(RX_DR) | _BV(TX_DS) | _BV(MAX_RT) );
  ce(HIGH);
  // Restore the pipe0 adddress, if exists
  if (pipe0_reading_address[0] > 0){
    RF24_write_register(RX_ADDR_P0, pipe0_reading_address, addr_width);	
  }else{
	RF24_closeReadingPipe(0);
  }

  // Flush buffers
  //flush_rx();
  if(RF24_read_register(FEATURE) & _BV(EN_ACK_PAY)){
	RF24_flushTx();
  }

  // Go!
  //delayMicroseconds(100);
}

/****************************************************************************/
static const uint8_t child_pipe_enable[] PROGMEM =
{
  ERX_P0, ERX_P1, ERX_P2, ERX_P3, ERX_P4, ERX_P5
};

void RF24_stopListening(void)
{  
  RF24_ce(LOW);

  delayMicroseconds(txRxDelay);
  
  if(RF24_read_register(FEATURE) & _BV(EN_ACK_PAY)){
    delayMicroseconds(txRxDelay); //200
	RF24_flushTx();
  }
  //flush_rx();
  RF24_write_register(CONFIG, ( RF24_read_register(CONFIG) ) & ~_BV(PRIM_RX) );
  RF24_write_register(EN_RXADDR, RF24_read_register(EN_RXADDR) | _BV(pgm_read_byte(&child_pipe_enable[0]))); // Enable RX on pipe0
  
  //delayMicroseconds(100);

}

/****************************************************************************/

void RF24_powerDown(void)
{
  RF24_ce(LOW); // Guarantee CE is low on powerDown
  RF24_write_register(CONFIG, RF24_read_register(CONFIG) & ~_BV(PWR_UP));
}

/****************************************************************************/

//Power up now. Radio will not power down unless instructed by MCU for config changes etc.
void RF24_powerUp(void)
{
   uint8_t cfg = RF24_read_register(CONFIG);

   // if not powered up then power up and wait for the radio to initialize
   if (!(cfg & _BV(PWR_UP))){
      RF24_write_register(CONFIG,RF24_read_register(CONFIG) | _BV(PWR_UP));

      // For nRF24L01+ to go from power down mode to TX or RX mode it must first pass through stand-by mode.
	  // There must be a delay of Tpd2stby (see Table 16.) after the nRF24L01+ leaves power down mode before
	  // the CEis set high. - Tpd2stby can be up to 5ms per the 1.0 datasheet
      delay(5);
   }
}

/******************************************************************/
#if defined (FAILURE_HANDLING) || defined (RF24_LINUX)
void RF24_errNotify(){
	#if defined (SERIAL_DEBUG) || defined (RF24_LINUX)
	  printf_P(PSTR("RF24 HARDWARE FAIL: Radio not responding, verify pin connections, wiring, etc.\r\n"));
	#endif
	#if defined (FAILURE_HANDLING)
	failureDetected = 1;
	#else
	delay(5000);
	#endif
}
#endif
/******************************************************************/

//Similar to the previous write, clears the interrupt flags
int RF24_write( const void* buf, uint8_t len, const int multicast )
{
	uint8_t status;
	//Start Writing
	startFastWrite(buf,len,multicast);

	//Wait until complete or failed
	#if defined (FAILURE_HANDLING) || defined (RF24_LINUX)
		uint32_t timer = millis();
	#endif 
	
	while( ! ( get_status()  & ( _BV(TX_DS) | _BV(MAX_RT) ))) { 
		#if defined (FAILURE_HANDLING) || defined (RF24_LINUX)
			if(millis() - timer > 85){			
				errNotify();
				#if defined (FAILURE_HANDLING)
				  return 0;		
				#else
				  delay(100);
				#endif
			}
		#endif
	}
    
	RF24_ce(LOW);

	status = RF24_write_register(NRF_STATUS,_BV(RX_DR) | _BV(TX_DS) | _BV(MAX_RT) );

  //Max retries exceeded
  if( status & _BV(MAX_RT)){
  	RF24_flushTx(); //Only going to be 1 packet int the FIFO at a time using this method, so just flush
  	return 0;
  }
	//TX OK 1 or 0
  return 1;
}

int RF24_write( const void* buf, uint8_t len ){
	return RF24_write(buf,len,0);
}
/****************************************************************************/

//For general use, the interrupt flags are not important to clear
int RF24_writeBlocking( const void* buf, uint8_t len, uint32_t timeout )
{
	//Block until the FIFO is NOT full.
	//Keep track of the MAX retries and set auto-retry if seeing failures
	//This way the FIFO will fill up and allow blocking until packets go through
	//The radio will auto-clear everything in the FIFO as long as CE remains high

	uint32_t timer = millis();							  //Get the time that the payload transmission started

	while( ( get_status()  & ( _BV(TX_FULL) ))) {		  //Blocking only if FIFO is full. This will loop and block until TX is successful or timeout

		if( get_status() & _BV(MAX_RT)){					  //If MAX Retries have been reached
			RF24_reUseTX();										  //Set re-transmit and clear the MAX_RT interrupt flag
			if(millis() - timer > timeout){ return 0; }		  //If this payload has exceeded the user-defined timeout, exit and return 0
		}
		#if defined (FAILURE_HANDLING) || defined (RF24_LINUX)
			if(millis() - timer > (timeout+85) ){			
				errNotify();
				#if defined (FAILURE_HANDLING)
				return 0;			
                #endif				
			}
		#endif

  	}

  	//Start Writing
	RF24_startFastWrite(buf,len,0);								  //Write the payload if a buffer is clear

	return 1;												  //Return 1 to indicate successful transmission
}

/****************************************************************************/

void RF24_reUseTX(){
		RF24_write_register(NRF_STATUS,_BV(MAX_RT) );			  //Clear max retry flag
		NRF24L01_TouchByte( REUSE_TX_PL );
		RF24_ce(LOW);										  //Re-Transfer packet
		RF24_ce(HIGH);
}

/****************************************************************************/

int RF24_writeFast( const void* buf, uint8_t len, const int multicast )
{
	//Block until the FIFO is NOT full.
	//Keep track of the MAX retries and set auto-retry if seeing failures
	//Return 0 so the user can control the retrys and set a timer or failure counter if required
	//The radio will auto-clear everything in the FIFO as long as CE remains high

	#if defined (FAILURE_HANDLING) || defined (RF24_LINUX)
		uint32_t timer = millis();
	#endif
	
	while( ( get_status()  & ( _BV(TX_FULL) ))) {			  //Blocking only if FIFO is full. This will loop and block until TX is successful or fail

		if( get_status() & _BV(MAX_RT)){
			//reUseTX();										  //Set re-transmit
			write_register(NRF_STATUS,_BV(MAX_RT) );			  //Clear max retry flag
			return 0;										  //Return 0. The previous payload has been retransmitted
															  //From the user perspective, if you get a 0, just keep trying to send the same payload
		}
		#if defined (FAILURE_HANDLING) || defined (RF24_LINUX)
			if(millis() - timer > 85 ){			
				errNotify();
				#if defined (FAILURE_HANDLING)
				return 0;							
				#endif
			}
		#endif
  	}
		     //Start Writing
	RF24_startFastWrite(buf,len,multicast);

	return 1;
}

int RF24_writeFast( const void* buf, uint8_t len ){
	return writeFast(buf,len,0);
}

/****************************************************************************/

//Per the documentation, we want to set PTX Mode when not listening. Then all we do is write data and set CE high
//In this mode, if we can keep the FIFO buffers loaded, packets will transmit immediately (no 130us delay)
//Otherwise we enter Standby-II mode, which is still faster than standby mode
//Also, we remove the need to keep writing the config register over and over and delaying for 150 us each time if sending a stream of data

void RF24_startFastWrite( const void* buf, uint8_t len, const int multicast, int startTx){ //TMRh20

	//write_payload( buf,len);
	RF24_write_payload( buf, len,multicast ? W_TX_PAYLOAD_NO_ACK : W_TX_PAYLOAD ) ;
	if(startTx){
		RF24_ce(HIGH);
	}

}

/****************************************************************************/

//Added the original startWrite back in so users can still use interrupts, ack payloads, etc
//Allows the library to pass all tests
void RF24_startWrite( const void* buf, uint8_t len, const int multicast ){

  // Send the payload

  //write_payload( buf, len );
  RF24_write_payload( buf, len,multicast? W_TX_PAYLOAD_NO_ACK : W_TX_PAYLOAD ) ;
  RF24_ce(HIGH);
  delayMicroseconds(10);
  RF24_ce(LOW);
}

/****************************************************************************/

int RF24_rxFifoFull(){
	return RF24_read_register(FIFO_STATUS) & _BV(RX_FULL);
}
/****************************************************************************/

int RF24_txStandBy(){

    #if defined (FAILURE_HANDLING) || defined (RF24_LINUX)
		uint32_t timeout = millis();
	#endif
	while( ! (RF24_read_register(FIFO_STATUS) & _BV(TX_EMPTY)) ){
		if( RF24_get_status() & _BV(MAX_RT)){
			RF24_write_register(NRF_STATUS,_BV(MAX_RT) );
			RF24_ce(LOW);
			RF24_flushTx();    //Non blocking, flush the data
			return 0;
		}
		#if defined (FAILURE_HANDLING) || defined (RF24_LINUX)
			if( millis() - timeout > 85){
				errNotify();
				#if defined (FAILURE_HANDLING)
				return 0;	
				#endif
			}
		#endif
	}

	RF24_ce(LOW);			   //Set STANDBY-I mode
	return 1;
}

/****************************************************************************/

int RF24_txStandBy(uint32_t timeout, int startTx){

    if(startTx){
	  RF24_stopListening();
	  RF24_ce(HIGH);
	}
	uint32_t start = millis();

	while( ! (RF24_read_register(FIFO_STATUS) & _BV(TX_EMPTY)) ){
		if( RF24_get_status() & _BV(MAX_RT)){
				RF24_write_register(NRF_STATUS,_BV(MAX_RT) );
				RF24_ce(LOW);										  //Set re-transmit
				RF24_ce(HIGH);
				if(millis() - start >= timeout){
					RF24_ce(LOW); RF24_flushTx(); return 0;
				}
		}
		#if defined (FAILURE_HANDLING) || defined (RF24_LINUX)
			if( millis() - start > (timeout+85)){
				errNotify();
				#if defined (FAILURE_HANDLING)
				return 0;	
				#endif
			}
		#endif
	}
	
	RF24_ce(LOW);				   //Set STANDBY-I mode
	return 1;

}

/****************************************************************************/

void RF24_maskIRQ(int tx, int fail, int rx){

	RF24_write_register(CONFIG, ( RF24_read_register(CONFIG) ) | fail << MASK_MAX_RT | tx << MASK_TX_DS | rx << MASK_RX_DR  );
}

/****************************************************************************/

uint8_t RF24_getDynamicPayloadSize(void)
{
  uint8_t result;
  result = NRF24L01_ReadByte( R_RX_PL_WID );
  if(result > 32) { RF24_flushRx(); delay(2); return 0; }
  return result;
}

/****************************************************************************/

int RF24_available(void)
{
  return RF24_available(NULL);
}

/****************************************************************************/

int RF24_available(uint8_t* pipe_num)
{
  if (!( RF24_read_register(FIFO_STATUS) & _BV(RX_EMPTY) )){

    // If the caller wants the pipe number, include that
    if ( pipe_num ){
	  uint8_t status = RF24_get_status();
      *pipe_num = ( status >> RX_P_NO ) & 0b111;
  	}
  	return 1;
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

void RF24_whatHappened(int& tx_ok,int& tx_fail,int& rx_ready)
{
  // Read the status & reset the status in one easy call
  // Or is that such a good idea?
  uint8_t status = RF24_write_register(NRF_STATUS,_BV(RX_DR) | _BV(TX_DS) | _BV(MAX_RT) );

  // Report to the user what happened
  tx_ok = status & _BV(TX_DS);
  tx_fail = status & _BV(MAX_RT);
  rx_ready = status & _BV(RX_DR);
}

/****************************************************************************/

/****************************************************************************/
void RF24_openWritingPipe(const uint8_t *address)
{
  // Note that AVR 8-bit uC's store this LSB first, and the NRF24L01(+)
  // expects it LSB first too, so we're good.

  RF24_write_register(RX_ADDR_P0,address, addr_width);
  RF24_write_register(TX_ADDR, address, addr_width);

  //const uint8_t max_payload_size = 32;
  //write_register(RX_PW_P0,rf24_min(payload_size,max_payload_size));
  RF24_write_register(RX_PW_P0,payload_size);
}

/****************************************************************************/
static const uint8_t child_pipe[] PROGMEM =
{
  RX_ADDR_P0, RX_ADDR_P1, RX_ADDR_P2, RX_ADDR_P3, RX_ADDR_P4, RX_ADDR_P5
};
static const uint8_t child_payload_size[] PROGMEM =
{
  RX_PW_P0, RX_PW_P1, RX_PW_P2, RX_PW_P3, RX_PW_P4, RX_PW_P5
};

/****************************************************************************/
void RF24_setAddressWidth(uint8_t a_width){

	if(a_width -= 2){
		write_register(SETUP_AW,a_width%4);
		addr_width = (a_width%4) + 2;
	}
}

/****************************************************************************/

void RF24_openReadingPipe(uint8_t child, const uint8_t *address)
{
  // If this is pipe 0, cache the address.  This is needed because
  // openWritingPipe() will overwrite the pipe 0 address, so
  // startListening() will have to restore it.
  if (child == 0){
    memcpy(pipe0_reading_address,address,addr_width);
  }
  if (child <= 6)
  {
    // For pipes 2-5, only write the LSB
    if ( child < 2 ){
      RF24_write_register(pgm_read_byte(&child_pipe[child]), address, addr_width);
    }else{
      RF24_write_register(pgm_read_byte(&child_pipe[child]), address, 1);
	}
    RF24_write_register(pgm_read_byte(&child_payload_size[child]),payload_size);

    // Note it would be more efficient to set all of the bits for all open
    // pipes at once.  However, I thought it would make the calling code
    // more simple to do it this way.
    RF24_write_register(EN_RXADDR, RF24_read_register(EN_RXADDR) | _BV(pgm_read_byte(&child_pipe_enable[child])));
  }
}

/****************************************************************************/

void RF24_closeReadingPipe( uint8_t pipe )
{
  RF24_write_register(EN_RXADDR,RF24_read_register(EN_RXADDR) & ~_BV(pgm_read_byte(&child_pipe_enable[pipe])));
}

/****************************************************************************/

void RF24_toggle_features(void)
{
	NRF24L01_WriteByte(ACTIVATE, 0x73);
}

/****************************************************************************/

void RF24_enableDynamicPayloads(void)
{
  // Enable dynamic payload throughout the system
    //toggle_features();
  RF24_write_register(FEATURE, RF24_read_register(FEATURE) | _BV(EN_DPL) );


  IF_SERIAL_DEBUG(printf("FEATURE=%i\r\n",read_register(FEATURE)));

  // Enable dynamic payload on all pipes
  //
  // Not sure the use case of only having dynamic payload on certain
  // pipes, so the library does not support it.
  RF24_write_register(DYNPD,RF24_read_register(DYNPD) | _BV(DPL_P5) | _BV(DPL_P4) | _BV(DPL_P3) | _BV(DPL_P2) | _BV(DPL_P1) | _BV(DPL_P0));

  dynamic_payloads_enabled = true;
}

/****************************************************************************/

void RF24_enableAckPayload(void)
{
  //
  // enable ack payload and dynamic payload features
  //

    //toggle_features();
  RF24_write_register(FEATURE,RF24_read_register(FEATURE) | _BV(EN_ACK_PAY) | _BV(EN_DPL) );

  IF_SERIAL_DEBUG(printf("FEATURE=%i\r\n",read_register(FEATURE)));

  //
  // Enable dynamic payload on pipes 0 & 1
  //

  RF24_write_register(DYNPD, RF24_read_register(DYNPD) | _BV(DPL_P1) | _BV(DPL_P0));
  dynamic_payloads_enabled = true;
}

/****************************************************************************/

void RF24_enableDynamicAck(void){
  //
  // enable dynamic ack features
  //
    //toggle_features();
  RF24_write_register(FEATURE,RF24_read_register(FEATURE) | _BV(EN_DYN_ACK) );

  IF_SERIAL_DEBUG(printf("FEATURE=%i\r\n",read_register(FEATURE)));
}

/****************************************************************************/

void RF24_writeAckPayload(uint8_t pipe, const void* buf, uint8_t len)
{
	NRF24L01_Write(W_ACK_PAYLOAD | ( pipe & 0b111 ), buf, len);
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

void RF24_setAutoAck(int enable)
{
  RF24_write_register(EN_AA,  enable ? 0b111111 : 0);
}

/****************************************************************************/

void RF24_setAutoAck( uint8_t pipe, int enable )
{
  if ( pipe <= 6 )
  {
    uint8_t en_aa = RF24_read_register( EN_AA ) ;
    if( enable )
    {
      en_aa |= _BV(pipe) ;
    }
    else
    {
      en_aa &= ~_BV(pipe) ;
    }
    RF24_write_register( EN_AA, en_aa ) ;
  }
}

/****************************************************************************/

int RF24RF24_testCarrier(void)
{
  return ( RF24_read_register(CD) & 1 );
}

/****************************************************************************/

int RF24_testRPD(void)
{
  return ( RF24_read_register(RPD) & 1 ) ;
}

/****************************************************************************/

void RF24_setPALevel(uint8_t level)
{

  uint8_t setup = RF24_read_register(RF_SETUP) & 0b11111000;

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
  int result = false;
  uint8_t setup = read_register(RF_SETUP) ;

  // HIGH and LOW '00' is 1Mbs - our default
  setup &= ~(_BV(RF_DR_LOW) | _BV(RF_DR_HIGH)) ;
  txRxDelay=250;
  if( speed == RF24_250KBPS )
  {
    // Must set the RF_DR_LOW to 1; RF_DR_HIGH (used to be RF_DR) is already 0
    // Making it '10'.
    setup |= _BV( RF_DR_LOW ) ;
    txRxDelay=450;
  }
  else
  {
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
    result = true;
  }
  return result;
}

/****************************************************************************/

rf24_datarate_e RF24_getDataRate( void )
{
  rf24_datarate_e result ;
  uint8_t dr = RF24_read_register(RF_SETUP) & (_BV(RF_DR_LOW) | _BV(RF_DR_HIGH));

  // switch uses RAM (evil!)
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
  uint8_t config = RF24_read_register(CONFIG) & ~( _BV(CRCO) | _BV(EN_CRC)) ;

  // switch uses RAM (evil!)
  if ( length == RF24_CRC_DISABLED )
  {
    // Do nothing, we turned it off above.
  }else if ( length == RF24_CRC_8 )
  {
    config |= _BV(EN_CRC);
  }else{
    config |= _BV(EN_CRC);
    config |= _BV( CRCO );
  }
  RF24_write_register( CONFIG, config ) ;
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

void RF24_disableCRC( void )
{
  uint8_t disable = RF24_read_register(CONFIG) & ~_BV(EN_CRC) ;
  RF24_write_register( CONFIG, disable ) ;
}

/****************************************************************************/
void RF24_setRetries(uint8_t delay, uint8_t count)
{
 RF24_write_register(SETUP_RETR,(delay&0xf)<<ARD | (count&0xf)<<ARC);
}
