
/*
 Copyright (C) 2011 J. Coliz <maniacbug@ymail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */
#ifndef __RF24_CONFIG_H__
#define __RF24_CONFIG_H__

  /*** USER DEFINES:  ***/  
  //#define FAILURE_HANDLING
  //#define SERIAL_DEBUG
  //#define MINIMAL
  
/**********************/
#define rf24_max(a,b) (a>b?a:b)
#define rf24_min(a,b) (a<b?a:b)

 

// Define _BV for non-Arduino platforms and for Arduino DUE
#include <stdint.h>
#include <stdio.h>
#include <string.h>


#define _BV(x) (1<<(x))
#ifdef SERIAL_DEBUG
	#define IF_SERIAL_DEBUG(x) ({x;})
#else
	#define IF_SERIAL_DEBUG(x)
#endif
  

// Progmem is Arduino-specific
// Arduino DUE is arm and does not include avr/pgmspace
 
typedef uint16_t prog_uint16_t;
#define PSTR(x) (x)
#define printf_P printf
#define strlen_P strlen  
#define PROGMEM
#define pgm_read_byte(addr) (*(const unsigned char *)(addr))
#define pgm_read_word(p) (*(p))
#define PRIPSTR "%s"

#endif // __RF24_CONFIG_H__

