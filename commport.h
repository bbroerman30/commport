//       ÉÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ»
//       º     COMMPORT.LIB (C) 1994, Nearly Perfect Software.     Ç¿
//       º                                                         º³
//       º               þ Written by Brad Broerman                º³
//       º               þ Last Modified: 05/04/94                 º³
//       º               þ Revision 1.0                            º³
//       ÈÍÑÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ¼³
//         ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

#ifndef COMMPORT
#define COMMPORT

#define MAX_COMM_BUFF_SIZE 5120

// These define the parity options for setting up the modem.
#define PAR_None 0
#define PAR_Even 1
#define PAR_Odd 2
#define PAR_Mark 3
#define PAR_Space 4

// These define the status bits for testing the line status register.
#define LS_DAVL 1      // Data available in the RDR
#define LS_OVRR 2      // Data Overrun
#define LS_PAR 4       // Parity Error
#define LS_FRAM 8      // Framing Error
#define LS_BREAK 16    // Break Detected.
#define LS_THRE 32     // Transmitter holding register empty.
#define LS_TMOT 128    // Timeout

// These define the status bits for testing the modem status register.
#define MS_CCTS 1      // CTS has changed
#define MS_CDSR 2      // DSR has changed
#define MS_CRI 4       // Ring Indicator hass changed.
#define MS_CDCD 8      // DCD has changed
#define MS_CTS 16      // CTS is currently high.
#define MS_DSR 32      // DSR is currently high.
#define MS_RI 64       // RI is currently high.
#define MS_DCD 128     // DCD is currently high.

class commport
 {
  //   The commport class accesses the serial port. It can use com1 through
  // com4, but only 1 at a time. You must close a commport before opening a
  // new one. It offers highly reliable communications through 57600bps. Input
  // is buffered in a 5k buffer, and output is unbuffered.
  //

  private:
  int TimeOut,
		TimeOutInt,
      Active,
		Save_PIC;
  char COMM_Port,
		 COMM_PortIRQ,
		 COMM_PortINT,
		 COMM_PortSettings,
		 COMM_Buffer[MAX_COMM_BUFF_SIZE];
  unsigned int COMM_PortBase,
					COMM_Buffer_Tail,
					COMM_Buffer_MaxSize,
					COMM_Buffer_Size,
					COMM_Buffer_Head,
					VectSeg,
					VectOff;
  unsigned char*CommInterruptHandler;


 public:
  commport(int Port=1,int Baud=2400, int Parity=PAR_None, int DATA=8, int STOP=1);
  ~commport();
  void init(int Port=1,int Baud=2400, int Parity=PAR_None, int DATA=8, int STOP=1); // Open the comm port.
  void initcustom(int Port, int Baud, int Parity, int DATA, int STOP, unsigned int Base, unsigned int IRQ, unsigned int INTR); // Opens port with custom base & irq.
  void close(); // Close the comm port.
  void chgsettings(int Parity=PAR_None, int Data=8, int Stop=1); // Change the communication settings.
  void chgbaud(int Baud); // Change the baud rate.
  int modstatus();        // Gets the status of the modem status register.
  int portstatus();       // Gets the status of the lline status register.
  int bad();              // Returns a 1 if an error occured during the last read/write.
  void setdsr(int S);     // If S is 1, sets DSR, else it clears DSR.
  void setrts(int S);     // If S is 1, sets RTS, else it clears RTS
  void setbreak(int S);   // If S is 1, sets Break, else it clears the break.
  void settmot(int S);    // Sets the timeout time to S (1/10th seconds).
  int bufferempty();      // Returns TRUE if the buffer is empty.
  char peek();            // Get the next byte from the buffer without extracting it.
  void flush();           // Emptys the input buffer.

// Extractors receive text form from port, and returns the types indicated.
  commport &operator>> (char &);  // Single character.
  commport &operator>> (char *);  // String.
  commport &operator>> (int &);   // Integer
  commport &operator>> (long &);  // Long Integer.
  commport &operator>> (double &); // Double precision float.

// Inserters take the types indicated and send the text form to the port.
  commport &operator<< (char &);  // Single character.
  commport &operator<< (char *);  // String.
  commport &operator<< (int &);   // Integer
  commport &operator<< (long &);  // Long Integer.
  commport &operator<< (double &); // Double precision float.
 };


#endif
