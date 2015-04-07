#include "stm32f4xx.h"
#include <stdio.h>

void printErrorMsg(const char * errMsg);
void printUsageErrorMsg(uint32_t CFSRValue);
void printBusFaultErrorMsg(uint32_t CFSRValue);
void printMemoryManagementErrorMsg(uint32_t CFSRValue);
void stackDump(uint32_t stack[]);


void printHex(uint32_t number)
{
	int i;
	uint8_t  nibble;
	printErrorMsg(" 0x");
	for(i = 0; i < 8; i++)
	{
		nibble = (number >> (28 - i*4)) & 0x0F;
		if(nibble <= 9) nibble += '0'; else nibble += ('A' - 10);
    fputc(nibble, stderr);
	}
	printErrorMsg(" \n");
}

void Hard_Fault_Handler(uint32_t stack[])
{
   //if((CoreDebug->DHCSR & 0x01) != 0) {
      printErrorMsg("In Hard Fault Handler\n");
      printErrorMsg("SCB->HFSR =");printHex(SCB->HFSR);
      if ((SCB->HFSR & (1 << 30)) != 0) {
         printErrorMsg("Forced Hard Fault\n");
         printErrorMsg("SCB->CFSR ="); printHex(SCB->CFSR );
         if((SCB->CFSR & 0xFFFF0000) != 0) {
            printUsageErrorMsg(SCB->CFSR);
         }
         if((SCB->CFSR & 0xFF00) != 0) {
            printBusFaultErrorMsg(SCB->CFSR);
         }
         if((SCB->CFSR & 0xFF) != 0) {
            printMemoryManagementErrorMsg(SCB->CFSR);
         }
      }
      stackDump(stack);
      __ASM volatile("BKPT #01");
   //}
   while(1);
}

int fputc(int c, FILE *f) {
  return ITM_SendChar(c);
}

void printErrorMsg(const char * errMsg)
{
   while(*errMsg != '\0'){
      fputc(*errMsg++, stderr);
   }
}

void printUsageErrorMsg(uint32_t CFSRValue)
{
   printErrorMsg("Usage fault: \n");
   CFSRValue >>= 16; // right shift to lsb

   if((CFSRValue & (1<<9)) != 0) {
      printErrorMsg("Divide by zero\n");
   }
}

void printBusFaultErrorMsg(uint32_t CFSRValue)
{
   printErrorMsg("Bus fault: \n");
   CFSRValue = ((CFSRValue & 0x0000FF00) >> 8); // mask and right shift to lsb
}

void printMemoryManagementErrorMsg(uint32_t CFSRValue)
{
   printErrorMsg("Memory Management fault: \n");
   CFSRValue &= 0x000000FF; // mask just mem faults
}

#if defined(__CC_ARM)
__asm void HardFault_Handler(void)
{
   TST lr, #4
   ITE EQ
   MRSEQ r0, MSP
   MRSNE r0, PSP
   B __cpp(Hard_Fault_Handler)
}
#elif defined(__ICCARM__)
void HardFault_Handler(void)
{
   __asm("TST lr, #4");
   __asm("ITE EQ");
   __asm("MRSEQ r0, MSP");
   __asm("MRSNE r0, PSP");
   __asm("B Hard_Fault_Handler");
}
#else
void HardFault_Handler(void) __attribute((naked));
void HardFault_Handler(void)
{
    __asm volatile
    (
       "TST lr, #4          \n"
       "ITE EQ              \n"
       "MRSEQ r0, MSP       \n"
       "MRSNE r0, PSP       \n"
       "B Hard_Fault_Handler \n"
    );
}
#endif


void stackDump(uint32_t stack[])
{
	 uint32_t r0, r1, r2, r3, r12, lr, pc, psr;
	 r0 = stack[0];
	 r1 = stack[1];
	 r2 = stack[2];
	 r3 = stack[3];
	 r12 = stack[4];
	 lr = stack[5];
	 pc = stack[6];
	 psr = stack[7];

   printErrorMsg("r0  ="); printHex(r0);
   printErrorMsg("r1  ="); printHex(r1);
   printErrorMsg("r2  ="); printHex(r2);
   printErrorMsg("r3  ="); printHex(r3);
   printErrorMsg("r12 ="); printHex(r12);
   printErrorMsg("lr  ="); printHex(lr);
   printErrorMsg("pc  ="); printHex(pc);
   printErrorMsg("psr ="); printHex(psr);
}

