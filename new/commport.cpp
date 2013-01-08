//       ษอออออออออออออออออออออออออออออออออออออออออออออออออออออออออป
//       บ     COMMPORT.LIB (C) 1994, Nearly Perfect Software.     วฟ
//       บ                                                         บณ
//       บ               þ Written by Brad Broerman                บณ
//       บ               þ Last Modified: 05/04/94                 บณ
//       บ               þ Revision 1.0                            บณ
//       ศอัอออออออออออออออออออออออออออออออออออออออออออออออออออออออผณ
//         ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤู

#include "comm_new.h"
#include <time.h>
#include <dos.h>
#include <stdlib.h>
#include <ctype.h>
#include <alloc.h>
#include <mem.h>

/*
	This is the ACTUAL ISR routine. It will be copied into an appropriate
  memory location, and the 0's will be filled with appropriate values so
  that the ISR's references point to the actual queue.
*/
unsigned char ISR[]={0x1E,0x60,0x9c,0xb8,0x00,0x00,0x8e,0xd8,0x8b,0x16,
		     0x00,0x00,0x42,0x32,0xc0                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                0x33,0xff,0x89,0x3e,0x00,0x00,0x8b,0x16,0x00,
		     0x00,0xec,0x88,0x01,0x8b,0x16,0x00,0x00,0x83,0xc2,
	             0x05,0xec,0xa8,0x01,0x75,0xc5,0xeb,0xaa,0x8b,0x16,
		     0x00,0x00,0x83,0xc2,0x04,0xec,0x0c,0x02,0xee,0x8b,
		     0x16,0x00,0x00,0x42,0xb0,0x01,0xee,0x9d,0x61,0x1f,
		     0xcf};
/*
  These are the bitmasks used to turn the requested interrupt on and off
*/
unsigned char INT_MASK_ON[]={0xfe,0xfd,0xfb,0xf7,0xef,0xdf,0xbf,0x7f};
unsigned char INT_MASK_OFF[]={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};

commport::commport(void)
 {
  TimeOut = TimeOutInt = 0;
  Active = 0;
  Save_PIC = 0;
  COMM_Port = 0;
  COMM_PortIRQ = 0;
  COMM_PortINT = 0;
  COMM_PortSettings = 0;
  COMM_PortBase = 0;
  COMM_Buffer_Tail = 0;
  COMM_Buffer_MaxSize = MAX_COMM_BUFF_SIZE;
  COMM_Buffer_Size = 0;
  COMM_Buffer_Head = 0;
  VectSeg = NULL;
  VectOff = NULL;
 };

commport::commport(int Port, int Baud, int Parity, int DATA, int STOP)
 {
  TimeOut = TimeOutInt = 0;
  Active = 0;
  Save_PIC = 0;
  COMM_Port = 0;
  COMM_PortIRQ = 0;
  COMM_PortINT = 0;
  COMM_PortSettings = 0;
  COMM_PortBase = 0;
  COMM_Buffer_Tail = 0;
  COMM_Buffer_MaxSize = MAX_COMM_BUFF_SIZE;
  COMM_Buffer_Size = 0;
  COMM_Buffer_Head = 0;
  VectSeg = NULL;
  VectOff = NULL;
  init(Port, Baud, Parity, DATA, STOP);
  setfifo(14);
 }

commport::~commport(void)
 {
  close();
 }

void commport::init(int Port, int Baud, int Parity, int DATA, int STOP)
 {
  unsigned int Base, IRQ, INTR;

  COMM_Port = Port;
  switch (COMM_Port)
	{
	 case 1:  Base = 0x3F8; IRQ  = 0x0C; INTR = 4; break;
	 case 2:  Base = 0x2F8; IRQ  = 0x0B; INTR = 3; break;
	 case 3:  Base = 0x3E8; IRQ  = 0x0C; INTR = 4; break;
	 case 4:  Base = 0x2E8; IRQ  = 0x0B; INTR = 3; break;
	 default: Base = 0x3F8; IRQ  = 0x0C; INTR = 4;

	}
  initcustom(Port, Baud, Parity, DATA, STOP, Base, IRQ, INTR);
 }

void commport::initcustom(int Port, int Baud, int Parity, int DATA, int STOP, unsigned int Base, unsigned int IRQ, unsigned int INTR)
 {
  unsigned char BRD_HI, 
                BRD_LO, 
                BIT_MASK, 
                Saved, 
                UCIrq, 
                Settings;
  unsigned int  Int_Seg, 
                Int_Off, 
                OLD_Seg, 
                OLD_Off, 
                PIC;

  COMM_PortBase = Base;
  COMM_PortIRQ = UCIrq = IRQ;
  COMM_PortINT = INTR;
  COMM_Port = Port;
  TimeOut = 0;
  TimeOutInt = 10;
  UART_type = uartype();
  Settings = COMM_PortSettings = (DATA-5) + 4*(STOP == 2);
  if (INTR > 7) // High interrupts (8-15)
   {
    BIT_MASK= (unsigned char)INT_MASK_ON[INTR-8];
    PIC = 0xa1;
   }
  else // Normal interupts.
   {
    BIT_MASK= (unsigned char)INT_MASK_ON[INTR];
    PIC = 0x21;
   }

  switch (Parity)
   {
    case 0:  break;
    case 1:  COMM_PortSettings += 32+16; break;
    case 2:  COMM_PortSettings += 32; break;
    case 3:  COMM_PortSettings += 32+8; break;
    case 4:  COMM_PortSettings += 32+8+16; break;
    default: COMM_PortSettings += 32+16;
   }

  switch (Baud)
   {
    case 300:  BRD_HI=0x01; BRD_LO=0x80; break;
    case 1200: BRD_HI=0x00; BRD_LO=0x60; break;
    case 2400: BRD_HI=0x00; BRD_LO=0x30; break;
    case 9600: BRD_HI=0x00; BRD_LO=0x0C; break;
    case 19200: BRD_HI=0x00; BRD_LO=0x06; break;
    case 38400: BRD_HI=0x00; BRD_LO=0x03; break;
    case 57600: BRD_HI=0x00; BRD_LO=0x02; break;
    default: BRD_HI=0x00; BRD_LO=0x30;
   }

  // Now set up the ISR routine...
  CommInterruptHandler = (unsigned char *)farmalloc(152);
  if (!CommInterruptHandler)
   return;
  _fmemcpy(CommInterruptHandler,ISR,151);

  // Now insert addresses for specific variables used in the ISR.
  
  // Insert the address of the Data Segment.
  CommInterruptHandler[4] = (unsigned char)(FP_SEG(&COMM_PortBase) & 0x00ff);
  CommInterruptHandler[5] = (unsigned char)(FP_SEG(&COMM_PortBase) >> 8);
  // Insert the offset of the COMM_PortBase variable.
  CommInterruptHandler[10] = CommInterruptHandler[44] = CommInterruptHandler[109]=
  CommInterruptHandler[116]= CommInterruptHandler[130]= CommInterruptHandler[141]=
  CommInterruptHandler[33] = CommInterruptHandler[79] = (unsigned char)(FP_OFF(&COMM_PortBase) & 0x00ff);
  CommInterruptHandler[11] = CommInterruptHandler[45] = CommInterruptHandler[110]=
  CommInterruptHandler[117]= CommInterruptHandler[131]= CommInterruptHandler[142]=
  CommInterruptHandler[34] = CommInterruptHandler[80] = (unsigned char)(FP_OFF(&COMM_PortBase) >> 8);
	// Insert the offset of the COMM_Buffer_Size variable.
  CommInterruptHandler[68] = CommInterruptHandler[85] = (unsigned char)(FP_OFF(&COMM_Buffer_Size) & 0x00ff);
  CommInterruptHandler[69] = CommInterruptHandler[86] = (unsigned char)(FP_OFF(&COMM_Buffer_Size) >> 8);
	// Insert the offset of the COMM_Buffer_MaxSize variable.
  CommInterruptHandler[73] = CommInterruptHandler[97] = (unsigned char)(FP_OFF(&COMM_Buffer_MaxSize) & 0x00ff);
  CommInterruptHandler[74] = CommInterruptHandler[98] = (unsigned char)(FP_OFF(&COMM_Buffer_MaxSize) >> 8);
	// Insert the offset of the actual COMM_Buffer.
  CommInterruptHandler[88] = (unsigned char)(FP_OFF(&COMM_Buffer) & 0x00ff);
  CommInterruptHandler[89] = (unsigned char)(FP_OFF(&COMM_Buffer) >> 8);
	// Insert the offset of the COMM_Buffer_Tail variable.
  CommInterruptHandler[92] = CommInterruptHandler[105] = (unsigned char)(FP_OFF(&COMM_Buffer_Tail) & 0x00ff);
  CommInterruptHandler[93] = CommInterruptHandler[106] = (unsigned char)(FP_OFF(&COMM_Buffer_Tail) >> 8);
  // Insert the offset of the COMM_PortINT variable.
  CommInterruptHandler[20] = (unsigned char)(FP_OFF(&COMM_PortINT) & 0x00ff);
  CommInterruptHandler[21] = (unsigned char)(FP_OFF(&COMM_PortINT) >> 8);

  // Now we get the address of the routine so we can insert it...
  Int_Seg = FP_SEG(CommInterruptHandler);
  Int_Off = FP_OFF(CommInterruptHandler);

asm { mov al,UCIrq   // Get the ports interrupt number
	mov ah,0x35           // Get the original interrupt vector
	int 0x21
	mov OLD_Off,bx        // Store the address of the original routine.
	mov OLD_Seg,es
	push ds
	mov al,UCIrq   // Get the ports interrupt number
	mov dx,Int_Off        // Move the offset into dx
	mov cx,Int_Seg        // Move the segment of the routine into ds
	mov ds,cx
	mov ah,0x25;
	int 0x21                // Set the new routine.
	pop ds
	mov dx,Base  // Point to the UART line control register.
	inc dx
	inc dx
	inc dx
	mov al,Settings // Send setup parameters
	out dx,al
	inc dx                   // Point to the modem control register.
	mov al,0x0b
	out dx,al                // Turn on interupts and DSR & RTS
	mov dx,PIC
	in al,dx               // Get PIC's interrupt mask
	mov Saved,al				 // Store the contents
	and al,BIT_MASK
	out dx,al              // Turn on the commports interrupts.
	mov dx,Base     // Point to the interrupt enable register.
	inc dx
	mov al,0x01              // Interrupt when data int RDR.
	out dx,al                // Send it.
	inc dx                   // Point to the line control register.
	inc dx
	in al,dx
	or al,0x80               // Set bit 7 high.
	out dx,al
	dec dx
	dec dx
	mov al,BRD_HI            // Store MSB of baud rate divisor.
	out dx,al
	dec dx
	mov al,BRD_LO            // Store LSB of baud rate divisor.
	out dx,al
	add dx,3
	in al,dx
	and al,0x7f              // Clear bit 7 of line control register.
	out dx,al
  }
  VectOff = OLD_Off;
  VectSeg = OLD_Seg;
  if (Saved & (unsigned char)INT_MASK_OFF[INTR])
	Save_PIC = 1;
  else
	Save_PIC = 0;
  Active = 1;
 }

void commport::close(void)
 {
  unsigned char BIT_MASK, Irq;
  unsigned int Base, Off, VSeg, PIC;

  if (!Active)
	return;
  Base = COMM_PortBase;
  Off = VectOff;
  VSeg = VectSeg;
  Irq = COMM_PortIRQ;
  if (COMM_PortINT > 7) // High interrupts (8-15)
	{
	 BIT_MASK= (unsigned char)INT_MASK_OFF[COMM_PortINT-8];
	 PIC = 0xa1;
	}
  else
	{
	 BIT_MASK= (unsigned char)INT_MASK_OFF[COMM_PortINT];
	 PIC = 0x21;
	}
  if (Save_PIC == 1)
	BIT_MASK= (unsigned char) 0x00;
  asm { mov dx,Base    // Point to the line control register
	add dx,4
	in al,dx
	and al,0xfC                  // Turn off DSR & RTS
	out dx,al
	mov dx,Base         // Point to the interrupt enable registe.
	inc dx
	mov al,0x00
	out dx,al                    // Turn off all interrupts.
	mov dx,PIC
	in al,dx                   // Get PIC's interrupt request mask.
	or al,BIT_MASK               // Turn off both comm ports.
	out dx,al
	push ds
	mov dx,Off               // Place offset into dx
	mov al,Irq
	mov cx,VSeg               // Get segment of original routine
	mov ds,cx                    // Put it into DS
	mov ah,0x25                  // Restore vector table.
	int 0x21
	pop ds
  }
  outportb(COMM_PortBase+2,0x00); // Disable the FIFOs (If enabled.)
  if (CommInterruptHandler)
	farfree(CommInterruptHandler);
  Active = 0;
 }

int commport::uartype(void)
 {
  unsigned char OldData = 0,
		Flag = 0;

  // Check for the presence of a UART.
  OldData = inportb(COMM_PortBase+4);
  outportb(COMM_PortBase+4,0x10);
  if ((inportb(COMM_PortBase+6) & 0xf0))
   return UART_NONE;
  outportb(COMM_PortBase+4,0x1f);
  if ((inportb(COMM_PortBase+6) & 0xf0) != 0xf0)
   return UART_NONE;
  outportb(COMM_PortBase+4,OldData);

  // Now check for a scratch register.
  OldData = inportb(COMM_PortBase+7);
  outportb(COMM_PortBase+7,0x55);
  if (inportb(COMM_PortBase+7) != 0x55)
   return UART_8250;
  outportb(COMM_PortBase+7,0xAA);
  if (inportb(COMM_PortBase+7) != 0xAA)
   return UART_8250;
  outportb(COMM_PortBase+7,OldData);

  // Now we check for the FIFOs
  outportb(COMM_PortBase+2,0x01);
  Flag = inportb(COMM_PortBase+2);
  outportb(COMM_PortBase+2,0x00);
  if ((Flag & 0x80) == 0) return UART_16450;

  // Finally, we check the FIFO Status flag for the 16550, or 16550A.
  if ((Flag & 0x40) == 0) return UART_16550;
  return UART_16550A;
 }

int commport::bufferempty(void)
 {
  return (COMM_Buffer_Size == 0);
 }

void commport::chgsettings(int Parity, int Data, int Stop)
 {
  unsigned int Base;
  unsigned char Settings;

  if (!Active)
	return;
  Base = COMM_PortBase;
  Settings = COMM_PortSettings = (Data-5) + 4*(Stop == 2);
  switch (Parity)
	{
	 case 1:  COMM_PortSettings += 32+16; break;
	 case 2:  COMM_PortSettings += 32; break;
	 case 3:  COMM_PortSettings += 32+8; break;
	 case 4:  COMM_PortSettings += 32+8+16; break;
	 default: COMM_PortSettings += 32+16;
	}
  asm{  mov dx,Base     // Point to LCR
	inc dx
	inc dx
	inc dx
	mov al,Settings  // Set data, stop, and parity.
	out dx,al
	  }
 }

void commport::chgbaud(int Baud)
 {
  unsigned char BRD_HI, BRD_LO;
  unsigned int Base;

  if (!Active)
	return;
  Base = COMM_PortBase;
  switch (Baud)
	{
	 case 300:  BRD_HI=0x01; BRD_LO=0x80; break;
	 case 1200: BRD_HI=0x00; BRD_LO=0x60; break;
	 case 2400: BRD_HI=0x00; BRD_LO=0x30; break;
	 case 9600: BRD_HI=0x00; BRD_LO=0x0C; break;
	 case 19200: BRD_HI=0x00; BRD_LO=0x06; break;
	 case 38400: BRD_HI=0x00; BRD_LO=0x03; break;
	 case 57600: BRD_HI=0x00; BRD_LO=0x02; break;
	 default: BRD_HI=0x00; BRD_LO=0x30;
	}
  asm { mov dx,Base    // Point to line control register
	inc dx
	inc dx
	inc dx
	in al,dx                // Get old setting.
	or al,0x80
	out dx,al               // Set bit 7 high.
	dec dx                  // Point to MSB of baud rate divisor.
	dec dx
	mov al,BRD_HI           // Send it.
	out dx,al
	dec dx                  // Point to LSB of baud rate divisor.
	mov al,BRD_LO
	out dx,al               // Send it.
	add dx,3                // Move back to the line control register.
	in al,dx
	and al,0x7f             // Clear bit 7
	out dx,al               // Send it.
		}
 }

int commport::setfifo(int Trig)
 {
  unsigned char Level;

  if (UART_type != UART_16550A)
	return -1;
  switch (Trig)
	{
	 case 1: Level = 0x07; break;
	 case 4: Level = 0x47; break;
	 case 8: Level = 0x87; break;
	 case 14: Level = 0xC7; break;
	 default: Level = 0x87;
	}
  outportb(COMM_PortBase+2,Level);
  return 1;
 }

int commport::modstatus(void)
 {
  unsigned char status;
  unsigned int Base;

  if (!Active)
	return 0;
  Base = COMM_PortBase;
  asm { mov dx,Base
	add dx,6
	in al,dx
	mov status,al
		}
  return ((int)status);
 }

int commport::portstatus(void)
 {
  unsigned char status;
  unsigned int Base;

  if (!Active)
	return -1;
  Base = COMM_PortBase;
  asm { mov dx,Base
	add dx,5
	in al,dx
	mov status,al
		}
  status += (TimeOut * 128); // Add a timeout bit,
  return ((int)status);
 }

int commport::bad(void)
 {
  if ((portstatus() & 158) != 0)
	return 0xffff;
  else
	return 0;
 }

void commport::setdsr(int S)
 {
  unsigned char B;

  if (!Active)
	return;
  B = inportb(COMM_PortBase+4);
  if (S)
	outportb(COMM_PortBase+4,B | 0x01);
  else
	outportb(COMM_PortBase+4,B & 0xfe);
 }

void commport::setrts(int S)
 {
  unsigned char B;

  if (!Active)
	return;
  B = inportb(COMM_PortBase+4);
  if (S)
	outportb(COMM_PortBase+4,B | 0x02);
  else
	outportb(COMM_PortBase+4,B & 0xfd);
 }

void commport::setbreak(int S)
 {
  unsigned char B;

  if (!Active)
	return;
  B = inportb(COMM_PortBase+3);
  if (S)
	outportb(COMM_PortBase+3,B | 0x40);
  else
	outportb(COMM_PortBase+3,B & 0xBf);
 }

void commport::settmot(int S)
 {
  TimeOutInt = S;
 }

char commport::peek(void)
 {
  long Ti,Tc;
  int temphead;
  char C = 0;

  Ti = clock();
  while (!COMM_Buffer_Size)  // If the buffer's empty, wait for up to 3 seconds.
	{
	 Tc = clock();
	 if (Tc > Ti+((TimeOutInt*CLK_TCK)/10)) break;
	}

  if (COMM_Buffer_Size) // If something's in it, get the byte.
	{
	 TimeOut = 0;
	 temphead = COMM_Buffer_Head;
	 ++temphead;
	 if (temphead == COMM_Buffer_MaxSize)
	  temphead = 0;
	 C = (unsigned char) COMM_Buffer[temphead];
	}                     // If it's empty, signal an error.
  else
	TimeOut = 1;
  return C;
 }

void commport::flush(void)
 {
  asm cli // Disable the interrupt.
  COMM_Buffer_Tail = 0; // Reset the buffer.
  COMM_Buffer_Size = 0;
  COMM_Buffer_Head = 0;
  asm sti // Enable interrupts.
 }

commport &commport::operator<< (char &C)
 {
  long Ti,Tc;
  int Error=0;

  if (!Active)
	return *this;
  Ti = clock();
  while (!(inportb(COMM_PortBase+5) & LS_THRE)) // wait until THR is empty.
	{
	 Tc = clock();
	 if (Tc > Ti+((TimeOutInt*CLK_TCK)/10))  { Error = 1; break; }    // Time-out after 3 seconds.
	}
  if (Error)
	TimeOut = 1;                                 // If error, return.
  else                                          // Otherwise, continue.
	{
	 TimeOut = 0;
	 outportb(COMM_PortBase,C); // Send the byte.
	}
  return *this;
 }

commport &commport::operator<< (char *S)
 {
  int i;

	for (i=0; *(S+i) != 0; ++i)
	 operator <<(*(S+i));
  return *this;
 }

commport &commport::operator<< (int &I)
 {
  char Strn[10];

  itoa(I,Strn,10);
  operator <<(Strn);
  return *this;
 }

commport &commport::operator<< (long &L)
 {
  char Strn[15];

  ltoa(L,Strn,10);
  operator <<(Strn);
  return *this;
 }

commport &commport::operator<< (double &D)
 {
  char Strn[40];

  gcvt(D,7,Strn);
  operator <<(Strn);
  return *this;
 }

commport &commport::operator>> (char &C)
 {
  long Ti,Tc;

  Ti = clock();
  while (!COMM_Buffer_Size)  // If the buffer's empty, wait for up to 3 seconds.
	{
	 Tc = clock();
	 if (Tc > Ti+((TimeOutInt*CLK_TCK)/10)) break;
	}

  if (COMM_Buffer_Size) // If something's in it, get the byte.
	{
	 TimeOut = 0;
	 ++COMM_Buffer_Head;
	 if (COMM_Buffer_Head == COMM_Buffer_MaxSize)
	  COMM_Buffer_Head = 0;
	 C = (unsigned char) COMM_Buffer[COMM_Buffer_Head];
	 --COMM_Buffer_Size;
	}                     // If it's empty, signal an error.
  else
	TimeOut = 1;
  return *this;
 }

commport &commport::operator>> (char *C) // Inputs a string.
 {
  char inchar;
  int index = 0;

  while (isspace(peek()))
   operator >> (inchar);       // Skip leading whitespace.
  while (!(isspace(peek())))
   {
    operator >> (inchar);      // Get characters until whitespace.
    *(C+index++) = inchar;
   }
  *(C+index) = 0x00;           // Null terminate end of string.
  return *this;
 }

commport &commport::operator >> (int &I)
 {
  char inchar, C[9];
  int index=0;

  while (isspace(peek()))      // Skip leading whitespace.
   operator >> (inchar);
  if ((peek() == '+') || (peek() == '-')) // Get sign character.
   {
    operator >> (inchar);
    *(C+index++) = inchar;
   }
  while ((isdigit(peek())) && (index < 8))// Get characters until no more digits.
   {
    operator >> (inchar);
    *(C+index++) = inchar;
   }
  *(C+index) = 0x00;           // Null terminate end of string.
  I =atoi(C);                  // Convert it to an int.
  return *this;
 }

commport &commport::operator >> (long &L)
 {
  char inchar, C[16];
  int index=0;

  while (isspace(peek()))      // Skip leading whitespace.
   operator >> (inchar);
  if ((peek() == '+') || (peek() == '-'))  // Get sign character.
   {
    operator >> (inchar);
    *(C+index++) = inchar;
   }
  while ((isdigit(peek())) && (index <15)) // Get characters until no more digits.
   {
    operator >> (inchar);
    *(C+index++) = inchar;
   }
  *(C+index) = 0x00;           // Null terminate end of string.
  L =atol(C);                  // Convert it to an int.
  return *this;
 }

commport &commport::operator >> (double &D)
 {
  char inchar, C[30], *endptr;
  int index=0;

  while (isspace(peek()))      // Skip leading whitespace.
   operator >> (inchar);
  if ((peek() == '+') || (peek() == '-'))  // Get sign character.
   {
    operator >> (inchar);
    *(C+index++) = inchar;
   }
  while (((isdigit(peek())) || (peek() == '.') || (peek() == 'e') ||
	 (peek() == 'E') || (peek() == '+') || (peek() == '-')) && (index <29)) // Get characters until no more digits.
   {
    operator >> (inchar);
	 *(C+index++) = inchar;
   }
  *(C+index) = 0x00;           // Null terminate end of string.
  D =strtol(C, &endptr, 10);  // Convert it to a double.
  return *this;
 }
