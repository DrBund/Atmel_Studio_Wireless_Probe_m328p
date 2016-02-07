                                            /* SPI ATMEGA radio */

/* This commit
 *  added PAYLOAD_WIDTH def, removed fixedPayloadWidth
 *
 *
 *
*/

// ------- Preamble -------- //
//#define F_CPU 1000000UL // 1 MHz
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "ATMEGA-328-pinDefines.h"
#include "USART.h"
#include "nRF24L01p.h"
#include "ds18b20.h"


//NRF24L01pClass * myRadio;
//NRF24L01p * myRadio;
NRF24L01p myRadio;
#define PAYLOAD_WIDTH 2

// Interrupt variable
volatile unsigned char IRQ_state = 0x00;

// Used to check the status of a given bit in a variable
#define CHECK_BIT(var,pos) ((var & (1 << pos)) == (1 << pos))

// GLOBALS >> GLOBALS  >> GLOBALS  >> GLOBALS  >> GLOBALS 
// Set if the radio is transmitter (TX) or receiver (RX)
int radioMode = 1; // radioMode = 1 for RX, 0 for TX
int rxDataFLAG = 0; // Indicates that there is data in the RX FIFO buffer
unsigned char signalVal = 0x05; // Dummy signal value for testing
unsigned char tmp_state [] = {0x00};
// GLOBALS << GLOBALS  << GLOBALS  << GLOBALS  << GLOBALS 

// Function declerations >>  Function declerations >> Function declerations
// void clear_interrupts(void);
void initInterrupt0(void);
void IRQ_reset_and_respond(void);
// Function declerations <<  Function declerations << Function declerations

void setup(void)
{
	/* add setup code here */
  initUSART();
	printString("Begin startup\r\n");
	
	// Start radio
	myRadio.begin();
	
	// Setup data pipes, addresses etc
	//
	// Use default addresses for now _ CHANGE ADDRESSES HERE IN FUTURE
	unsigned char pipesOn [] = {0x01}; // which pipes to turn on for receiving
	myRadio.setup_data_pipes(pipesOn, PAYLOAD_WIDTH);
	
	//DEBUG - change RX_ADDR_P0 to see if I am reading the right value
	unsigned char tmpArr [] = {0xE7,0xE7,0xE7,0xE7,0xE7};
	myRadio.writeRegister(RX_ADDR_P0,tmpArr, 5);
	
	// Configure radio to be TX (transmitter) or RX (receiver)
	printString("Configure Radio\r\n");
	if (radioMode)
	{myRadio.rMode();}// Configure radio to be a receiver
	else
	{myRadio.txMode();}// Configure radio to be a receiver

	// Clear any interrupts
	myRadio.clear_interrupts();

	
	tmp_state[0] = *myRadio.readRegister(STATUS, 0);
	printString("STATUS: ");
	printBinaryByte(tmp_state[0]);
	printString("\r\n");

	
	myRadio.setDebugVal(123);
	printString("**************************************  debug_val  = ");
	printWord(myRadio.getDebugVal());
	printString("\r\n");

	// Attach interrupt 0 (digital pin 2 on the Arduino Uno)
	//	This interrupt monitors the interrupt pin (IRQ) from the nRF24L01 Radio
	//  The IRQ is normally high, and active low
	//  The IRQ is triggered at:
	_delay_ms(100); // Make sure all the configuration is completed before attaching the interrupt
	//attachInterrupt(0, IRQ_resolve, FALLING);
  // The nRF24L01p chip should pullup the pin when not interrupting
  
  
  tmp_state[0] = *myRadio.readRegister(SETUP_RETR, 0);
  printString("SETUP_RETR: ");
  printBinaryByte(tmp_state[0]);
  printString("\r\n");initInterrupt0();
  
  tmp_state [0] = 1<<(ARC)|1<<(ARC+1)|1<<(ARC+2)|1<<(ARC+3);
  myRadio.writeRegister(SETUP_RETR, tmp_state, 1);
  
  tmp_state[0] = *myRadio.readRegister(CONFIG, 0);
  printString("CONFIG: ");
  printBinaryByte(tmp_state[0]);
  printString("\r\n");
  tmp_state[0] = *myRadio.readRegister(EN_AA, 0);
  printString("EN_AA: ");
  printBinaryByte(tmp_state[0]);
  printString("\r\n");
  tmp_state[0] = *myRadio.readRegister(SETUP_RETR, 0);
  printString("SETUP_RETR: ");
  printBinaryByte(tmp_state[0]);
  printString("\r\n");
}

void initInterrupt0(void)
{
  EIMSK |= (1 << INT0);                  /* Enable INT0 */
  EICRA |= (1<< ISC01);         /* trigger when falling */
  sei();             /* set global interrupt enable bit */
}

int main(void) {

  // -------- Inits --------- //
  setup();
  
  // ------ Event loop ------ //
  while (1) {
	  
	/*
	// For DEBUG
	//myRadio.clear_interrupts();
	tmp_state[0] = *myRadio.readRegister(STATUS, 0);
	printString("STATUS: ");
	printBinaryByte(tmp_state[0]);
	printString("\r\n");
	*/
	
    /* add main program code here */
    int serialCommand = 0; // Serial commands: 0-do nothing
                 //                  1-query radioSlave for data
    int serialData	  = 0; // Data byte
	
	

    if (IRQ_state == 1)
    {
      IRQ_reset_and_respond();
    }
    
    // Radio is in TX mode
    if (radioMode == 0) 
    {
      // Send data
      printString("Transmit code abc go\r\n");
      unsigned char tmpData [] = {1,2,3,4,26}; // Data needs to be the same size as the fixedDataWidth set in setup
      myRadio.txData(tmpData, 5); // This is currently sending data to pipe 0 at the default address. Change this once the radio is working
      _delay_ms(2000);
    }
    // Radio is in RX mode
    // Receive transmission from master
    else
    {
      if (rxDataFLAG == 1)
      {
        // Receive data and print
        unsigned char * tmpRxData = myRadio.rData(2);
        serialCommand = *(tmpRxData+0);
        serialData    = *(tmpRxData+1);
		myRadio.clear_interrupts();
		/*
		printString("rxDataFLAG set\r\n");
		printString("serialCommand: ");
		printBinaryByte(serialCommand);
		printString("\r\n");
		printString("serialData: ");
		printBinaryByte(serialData);
		printString("\r\n");
		*/
		
        // Check Command byte
        if(serialCommand == 0X01) // Read signal and return data to master
        {
          //signalVal++; // Dummy signal value for testing
          // Turn Master to transmitter
          myRadio.txMode();
		  //_delay_ms(200); // Delay to allow for the master to enter itself into receiver mode so it can catch this message
		  /*tmp_state[0] = *myRadio.readRegister(CONFIG, 0);
		  printString("CONFIG tx: ");
		  printBinaryByte(tmp_state[0]);
		  printString("\r\n"); */
		  // MISO Command byte for return signal value: 0x02
          //unsigned char tmpData [] = {0x02, signalVal}; // Data needs to be the same size as the fixedDataWidth set in setup
		  
		  //unsigned char tmpData [] = {0x02, 0x01}; // Data needs to be the same size as the fixedDataWidth set in setup
          // START HERE: does not work if I comment out these lines and replace with dummy variable, does the radio not have time to enter txMode?
		  double d = 0;
		  d = ds18b20_gettemp();
		  unsigned char tmpData [] = {2, (unsigned char)d}; // Data needs to be the same size as the fixedDataWidth set in setup  
		  //unsigned char tmpData [] = {2, 3};
		  myRadio.txData(tmpData, 2); // This is currently sending data to pipe 0 at the default address. Change this once the radio is working
		 
		  
		  // Loop until packet is transmitted
		  uint8_t packetTransmitted = 0;
		  uint8_t tmpInd = 0;
		  while(packetTransmitted == 0)
		  {
			  tmp_state[0] = *myRadio.readRegister(FIFO_STATUS, 0);
			  packetTransmitted = (tmp_state[0] & (1<<TX_EMPTY));
			  tmpInd++;
			  if(tmpInd > 200)
			  {
				  printString("Transmission retries maxed out\r\n");
				  break;
			  }
			  _delay_ms(10); // If this is set to <=2ms the radio does not work very well (Start Here, look into retransmit without loop?)
		  }
		  
	 
		  //_delay_ms(400);
          // Turn Master to receiver
          myRadio.rMode();
		  
		  // Check lost packets
		  /*
		  tmp_state[0] = *myRadio.readRegister(FIFO_STATUS, 0);
		  printString("FIFO_STATUS: ");
		  printBinaryByte(tmp_state[0]);
		  printString("\r\n");
		  tmp_state[0] = *myRadio.readRegister(OBSERVE_TX, 0);
		  printString("OBSERVE_TX: ");
		  printBinaryByte(tmp_state[0]);
		  printString("\r\n");
		  */
		  
        }
        /*
        printString("RX Data: \r\n");
        for (int x=0; x<2; x++)
        {
          printString("Element ");
          printWord(x);
          printString(": ");
          printWord(*(tmpRxData+x));
          printString("\r\n");
        }
        */
        //myRadio.flushRX();
        myRadio.flushRX();
        rxDataFLAG = 0; // reset rxDataFLAG
      }
      
      
    }
	

    //_delay_ms(5); // Short delay to keep everything running well. Make sure the IRQ's get cleared before next loop. etc...
	//_delay_ms(2000); // Short delay to keep everything running well. Make sure the IRQ's get cleared before next loop. etc...
	


  }                                                  /* End event loop */
  return (0);                            /* This line is never reached */
}




unsigned char setBit(unsigned char byteIn, int bitNum, bool setClear)
{
	if(setClear == 1)
	byteIn |= (1<<bitNum);
	else
	byteIn &= ~(1<<bitNum);
	
	return byteIn;
}



//******* INTERRUPTS **************** INTERRUPTS ***************** INTERRUPTS ****************************

/* IRQ_resolve
Resolve the attachInterrupt function quickly
*/
ISR(INT0_vect)
{
	// Get the IRQ code from the receiver and assign it to IRQ_state variable
	//unsigned char * p_tmp;
	//printString("IRQ");
	//IRQ_state = * myRadio.readRegister(STATUS,1); // this returns a pointer, so I dereferenced it to the unsigned char for IRQ_state
	IRQ_state = 1;
	//myRadio.clear_interrupts();
}



/* IRQ_reset_and_respond
Reset the IRQ in the radio STATUS register
Also resolve the condition which triggered the interrupt
*/
void IRQ_reset_and_respond(void)
{
	
	
	//printString(" ------------------ RESPOND TO IRQ --------------------- \r\n");

	tmp_state[0] = *myRadio.readRegister(STATUS, 0);
	//printString("STATUS: ");
	//printBinaryByte(tmp_state[0]);
	//printString("\r\n");
	
	myRadio.clear_interrupts();
	IRQ_state = 0; //reset IRQ_state
	
	if CHECK_BIT(tmp_state[0],0) // TX_FIFO full
	{
		printString("TX_FIFO Full\r\n");
	}
	//if (CHECK_BIT(tmp_state[0],1)|CHECK_BIT(tmp_state[0],2)|CHECK_BIT(tmp_state[0],3)) // Pipe Numbers
	//{
	//	printString("Pipe Number Changed\r\n");
	//}
	if CHECK_BIT(tmp_state[0],4) // Maximum number of TX retries interrupt
	{
		printString("Max TX retries IRQ\r\n");
		myRadio.flushTX();
	}
	if CHECK_BIT(tmp_state[0],5) // Data sent TX FIFO interrupt
	{
		printString("Data Sent TX FIFO IRQ\r\n");
	}
	if CHECK_BIT(tmp_state[0],6) // Data ready RX FIFO interrupt
	{
		//printString("Data ready RX FIFO IRQ\r\n");
		// Read the data from the R_RX_PAYLOAD
		// RX_P_NO bits 3:1 tell what pipe number the payload is available in 000-101: Data Pipe Number, 110: Not Used, 111: RX_FIFO Empty
		// Get bits 3:1 and right shift to get pipe number
		//pipeNumber = (tmp_state[0] & 0xE) >> 1;
		rxDataFLAG = 1; //Set Rx Data FLAG
	}
	
	
	
}

/*
void clear_interrupts(void)
{
	// Clear any interrupts
	unsigned char tmp_state [] = {1<<RX_DR};
	myRadio.writeRegister(STATUS, tmp_state, 1);
	tmp_state [0] = 1<<TX_DS;
	myRadio.writeRegister(STATUS, tmp_state, 1);
	tmp_state [0] = 1<<MAX_RT;
	myRadio.writeRegister(STATUS, tmp_state, 1);
	// Flush the TX register
	myRadio.flushTX();
}
*/

