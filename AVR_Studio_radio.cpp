                                            /* SPI ATMEGA radio */

/* This commit
 *  cleaned up code, removed comments 2-7-2016 SL
 *
 *
 *
*/

// >> DEBUG >> DEBUG >> DEBUG >> DEBUG >> DEBUG 
	/*
	//myRadio.clear_interrupts();
	tmp_state[0] = *myRadio.readRegister(STATUS, 0);
	printString("STATUS: ");
	printBinaryByte(tmp_state[0]);
	printString("\r\n");
	*/
	
	/*
	tmp_state[0] = *myRadio.readRegister(CONFIG, 0);
	printString("CONFIG tx: ");
	printBinaryByte(tmp_state[0]);
	printString("\r\n"); 
	*/
// >> DEBUG >> DEBUG >> DEBUG >> DEBUG >> DEBUG

// ------- Preamble -------- //
//#define F_CPU 1000000UL // 1 MHz
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "ATMEGA-328-pinDefines.h"
#include "USART.h"
#include "nRF24L01p.h"
#include "ds18b20.h"

#define PAYLOAD_WIDTH 3

//NRF24L01pClass * myRadio;
//NRF24L01p * myRadio;
NRF24L01p myRadio;

// Interrupt variable
volatile unsigned char rxDataFLAG = 0; // Indicates that there is data in the RX FIFO buffer
volatile unsigned char tx_DS_FLAG = 0; // Data sent TX FIFO interrupt
volatile unsigned char max_RT_FLAG = 0; // Max retries interrupt

// Used to check the status of a given bit in a variable
#define CHECK_BIT(var,pos) ((var & (1 << pos)) == (1 << pos))

// GLOBALS >> GLOBALS  >> GLOBALS  >> GLOBALS  >> GLOBALS 
// Set if the radio is transmitter (TX) or receiver (RX)
int radioMode = 1; // radioMode = 1 for RX, 0 for TX
unsigned char signalVal = 0x05; // Dummy signal value for testing
unsigned char tmp_state [] = {0x00};
// GLOBALS << GLOBALS  << GLOBALS  << GLOBALS  << GLOBALS 

// Function declerations >>  Function declerations >> Function declerations
// void clear_interrupts(void);
void initInterrupt0(void);
void IRQ_reset_and_respond(void);
void initADC1(void);
uint16_t readADC(uint8_t channel);
void radio_tx_double(uint8_t serialCommand, uint16_t txWord);
// Function declerations <<  Function declerations << Function declerations

// SENSORS >> SENSORS >> SENSORS >> SENSORS >> SENSORS  
//    Add sensor values here for DS18B20
//
//    Soil moisture sensor
//      Defined in ATMEGA-328-pinDefines.h as MOIST_SENSOR
//      Currently on pin PC1 / ADC1
//
// SENSORS << SENSORS << SENSORS << SENSORS << SENSORS  

void initADC1()
{
  ADMUX |= (1 << REFS0);                     /* reference voltage on AVCC */
  ADCSRA |= (1 << ADPS1) | (1 << ADPS0);        /* ADC clock prescaler /8 */
  ADCSRA |= (1 << ADEN);                                    /* enable ADC */
  printString("ADC1 initialized\r\n");
}

uint16_t readADC(uint8_t channel)
{
  ADMUX = (0xf0 & ADMUX) | channel;                     /* select ADC pin */
  ADCSRA |= (1 << ADSC);                          /* start ADC conversion */
  loop_until_bit_is_clear(ADCSRA, ADSC);               /* loop until done */
  return (ADC);
}

void radio_tx_double(uint8_t serialCommand, uint16_t txWord)
{
  // SL_1 changed uintLSB and uintMSB from uint8_t to unsigned char
  unsigned char uintLSB = 0;
  unsigned char uintMSB = 0;

  // Bit shift data out of word and into u char
  uintLSB = (txWord & 0b11111111);
  txWord = (txWord >> 8);
  uintMSB = (txWord & 0b11111111);

  //unsigned char tmpData [] = {serialCommand, uintMSB, uintLSB}; /* Data needs to be the 
  //                                                  same size as the 
  //                                                  fixedDataWidth set in 
  //                                                  setup */
  
  unsigned char tmpData [] = {serialCommand, uintLSB, uintMSB};
	  
  //printString("transmit data from within radio_tx_double \r\n");
  myRadio.txData(tmpData, PAYLOAD_WIDTH);             /* This is currently sending data 
                                                      to pipe 0 at the default address. 
                                               Change this once the radio is working */
}

void setup(void)
{
	/* add setup code here */
  initUSART();
	printString("Begin startup\r\n");

	// Begin SPI communication, initialized in nRF24L01.cpp constructor
	//initSPImaster();
  //
	// int airDataRate = 250; //kBps, can be 250, 1000 or 2000 section 6.3.2
	//int rfChannelFreq = 0x02; // = 2400 + RF_CH(MHz) section 6.3.3 0x02 is the default
	//  RF_CH can be set from 0-83. Any channel higher than 83 is off limits in US by FCC law
	//SETUP_AW: AW=11-5 byte address width
	//myRadio = new NRF24L01pClass;
	//myRadio = new NRF24L01p(SPI_CE,SPI_CSN);
	//myRadio.init(SPI_CE,SPI_CSN);
	//myRadio = new NRF24L01p;
	//NRF24L01p myRadio;

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
  printString("\r\n");
  
  initInterrupt0();
  
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
  uint16_t adcValue;                   /* hold value for ADC */
  initADC1();                          /* initialize the ADC */

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
    // SL_1 - changed serialCommand and serialData from int to uchar
    unsigned char serialCommand = 0; // Serial commands: 0-do nothing
                 //                  1-query radioSlave for data
    unsigned char serialData1	  = 0; // Data byte
    unsigned char serialData2	  = 0; // Data byte
	
	

    // Radio is in TX mode
    if (radioMode == 0) 
    {
      // Send data
      printString("Transmit code abc go\r\n");
      unsigned char tmpData [] = {1,2,3}; // Data needs to be the same size as the fixedDataWidth set in setup
      myRadio.txData(tmpData, PAYLOAD_WIDTH); // This is currently sending data to pipe 0 at the default address. 
								  // Change this once the radio is working
      //_delay_ms(2000);
    }
    // Radio is in RX mode
    // Receive transmission from master
    else
    {
      if (rxDataFLAG == 1)
      {
        // Receive data and print
        unsigned char * tmpRxData = myRadio.rData(PAYLOAD_WIDTH);
        serialCommand = *(tmpRxData+0);
        serialData1   = *(tmpRxData+1);
        serialData2   = *(tmpRxData+2);
		//myRadio.clear_interrupts(); // TODO: does this need to be here?
	      /* DEBUG - do not leave on during operation
          printString("serialCommand: ");
          printBinaryByte(serialCommand);
          printString("\r\n");
          printString("serialData1: ");
          printBinaryByte(serialData1);
          printString("\r\n");
		      printString("serialData2: ");
		      printBinaryByte(serialData2);
		      printString("\r\n");
        */




        // Check Command byte
        // --------------------------------------------------------------
        // Send DS18B20 temperature value
        // --------------------------------------------------------------
        if(serialCommand == 0X01) // Read signal and return data to master
        {
          // Turn Master to transmitter
          myRadio.txMode();
          
          double d = 0;
          d = ds18b20_gettemp();

          //unsigned char tmpData [] = {2, (unsigned char)d,3}; // Data needs to be the 
                                                            // same size as the 
                                                            // fixedDataWidth set in setup  
                                                            //
          //myRadio.txData(tmpData, PAYLOAD_WIDTH); // This is currently sending data to pipe 0 at the 
          
          radio_tx_double(0x02, d); // This is currently sending data to pipe 0 at the 
                                      // default address. Change this once the 
                                      // radio is working
		 
          /* Read moisture meater
          adcValue = readADC(MOIST_SENSOR); 
          radio_tx_double(adcValue)
		      */
          
          // Loop until packet is transmitted
          uint8_t packetTransmitted = 0;
          uint8_t tmpInd = 0;
          while(packetTransmitted == 0)
          {
            tmp_state[0] = *myRadio.readRegister(FIFO_STATUS, 0);
            packetTransmitted = (tmp_state[0] & (1<<TX_EMPTY));
            tmpInd++;
            if(tmpInd > 10)
            {
              printString("Transmission retries maxed out temp\r\n");
              break;
            }
            _delay_ms(200); // If this is set to <=2ms the radio does not work very well (Start Here, look into retransmit without loop?)
          }
          
          // Turn Master to receiver
		  tx_DS_FLAG = 0;
          myRadio.rMode();
		  
		  
        }
        


        // --------------------------------------------------------------
        // Send ADC value
        // --------------------------------------------------------------
        if(serialCommand == 0X03) // Read signal and return data to master
        {
          // Turn Master to transmitter
          myRadio.txMode();
          
          // Read moisture meter
          printString("Read ADC: ");
          adcValue = readADC(MOIST_SENSOR); 
          printWord(adcValue);
          printString("\r\n");

          radio_tx_double(0x04, adcValue);
         
          // Loop until packet is transmitted
		  //   Transmission usually takes ~800-1000us for a double
		  //   myRadio.readRegister has to be polled for transmission to go through
		  //   not sure why this is, it can be any register, not just STATUS
		  //   FIFO_STATUS register is used here to monitor if the packet has been sent.
		  //   The register must also be polled at intervals in the 100's of microseconds
		  //   You can't just poll the register 10x in a loop with a 10us delay and carry on.
          uint8_t packetTransmitted = 0;
          uint8_t tmpInd = 0;
          while(packetTransmitted == 0)
          {
            //printString("tmpInd: ");
            //printByte(tmpInd);
            //printString("\r\n");
			
			tmp_state[0] = *myRadio.readRegister(FIFO_STATUS, 0);
            packetTransmitted = (tmp_state[0] & (1<<TX_EMPTY));
			
            tmpInd++;
            if(tmpInd > 100)
            {
              printString("Transmission retries maxed out ADC\r\n");
              break;
            }
            _delay_ms(200); // If this is set to <=2ms the radio does not work very well (Start Here, look into retransmit without loop?)
          }
		  
		  
          
          // Turn Master to receiver
		  tx_DS_FLAG = 0;
          myRadio.rMode();
		  
        }
       
        myRadio.flushRX(); //TODO: does this need to be here?
        rxDataFLAG = 0; // reset rxDataFLAG
      }
      
      
    }

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
	//printString(" ------------------ RESPOND TO IRQ --------------------- \r\n");

	tmp_state[0] = *myRadio.readRegister(STATUS, 0);
	//printString("STATUS: ");
	//printBinaryByte(tmp_state[0]);
	//printString("\r\n");
	
	myRadio.clear_interrupts();
	
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
		max_RT_FLAG = 1;
		printString("Max TX retries IRQ\r\n");
		myRadio.flushTX();
	}
	if CHECK_BIT(tmp_state[0],5) // Data sent TX FIFO interrupt
	{
		tx_DS_FLAG = 1;
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


