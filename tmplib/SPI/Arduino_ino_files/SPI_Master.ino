/* Bio SPI Master Arduino 
 *
 * Controls print head for biological materials for RepRap printer.
 *
 * SPI pin numbers:
 * SCK   13  // Serial Clock.
 * MISO  12  // Master In Slave Out.
 * MOSI  11  // Master Out Slave In.
 * SS    10  // Slave Select - not used for master
*/

#include <avr.io>
#include "SPI_pins.h"
#include "SPI.h"

// Pin Definitions

// Useful functions
                                 /* Which pin selects BIOSPI as slave? */
#define SLAVE_SELECT    digitalWrite(USI_SS_PIN, LOW)
#define SLAVE_DESELECT  digitalWrite(USI_SS_PIN, HIGH)

// Instruction Set -- interface with Arduino  Not currently used
#define SPI_LED_ON    0b00000011                     /* turn on LED */
#define SPI_LED_OFF   0b00000010                    /* turn off LED */

#define SPI_RDSR      0b00000101            /* read status register */
#define SPI_WRSR      0b00000001           /* write status register */

/* Define Pins */

void masterInitSPI(void) {
  // Initialize SPI pins
  pinMode(MISO, INPUT);
  pinMode(MOSI, OUTPUT);
  pinMode(SCK, OUTPUT);
  pinMode(SS, OUTPUT);
  pinMode(USI_SS_PIN, OUTPUT);
  digitalWrite(USI_SS_PIN, HIGH); // Chip deselected on HIGH
  
  /* Don't have to set phase, polarity b/c
   * default should work with Arduino */
  SPCR |= (1 << SPR1);      /* div 16, safer for breadboards */
  SPCR |= (1 << MSTR);                        /* clockmaster */
  SPCR |= (1 << SPE);                              /* enable */
}
/*
void slaveSelect() {
  digitalWrite(USI_SS_PIN, LOW); 
}

void slaveDeselect() {
  digitalWrite(USI_SS_PIN, HIGH); 
}
*/

uint8_t SPI_tradeByte(uint8_t byte) {
  SPDR = byte;                       /* SPI starts sending immediately */
  loop_until_bit_is_set(SPSR, SPIF);                /* wait until done */
                                /* SPDR now contains the received byte */
  return(SPDR);
}

uint8_t SPI_readByte() {
  SLAVE_SELECT;
  //slaveSelect();
  uint8_t tmp_rx = SPI_tradeByte(0);
  //slaveDeselect();
  SLAVE_DESELECT;
  return (tmp_rx);
}

void SPI_writeByte(uint8_t byte) {
  SLAVE_SELECT;
  //slaveSelect();
  SPI_tradeByte(byte);
  //slaveDeselect();
  SLAVE_DESELECT;
}

void SPI_turnOnLED() {
    //slaveSelect();
    SLAVE_SELECT;
  // Send serial byte to SPI slave
    SPI_tradeByte(SPI_LED_ON);
  // Check reposinse to confirm message was recieved
    uint8_t rx_byte = SPI_tradeByte(0);
    if (rx_byte == SPI_LED_ON) {
      Serial.println("SPI_LED_ON command recieved");
    }
    else {
      Serial.println("Error: Command either incorrect or miscommunicated");
    } 
    SLAVE_DESELECT;
    //slaveDeselect();
}

void SPI_turnOffLED() {
    //slaveSelect();
    SLAVE_SELECT;
  // Send serial byte to SPI slave
    SPI_tradeByte(SPI_LED_OFF);
  // Check reposinse to confirm message was recieved
    uint8_t rx_byte = SPI_tradeByte(0);
    if (rx_byte == SPI_LED_OFF) {
      Serial.println("SPI_LED_OFF command recieved");
    }
    else {
      Serial.println("Error: Command either incorrect or miscommunicated");
    } 
    SLAVE_DESELECT;
    //slaveDeselect();
}


void setup() {
  // Initialize serial for troubleshooting.
  Serial.begin(9600);
  // Initialize SPI Slave.
  masterInitSPI();
  Serial.println("Master Initialized");
}


void loop() {
  
  // Use Serial to turn on / off LED by sending commands over SPI

  //if (Serial.available()) {
    /*
    uint8_t incomming_serial = Serial.read();

    Serial.print("Serial in: ");
    Serial.println(incomming_serial);
    */
    
    delay(1000);
    //Serial.println("Write SPI_LED_ON");
    SPI_turnOnLED();
    
    delay(3000);
    SPI_turnOffLED();
    //SPI_writeByte(SPI_LED_OFF);
    
    
    // Get reply from slave
    //uint8_t rtnByte = SPI_readByte();
    //Serial.print("Slave response: ");
    //Serial.println(rtnByte);
    
    //delay(500);

    // Read what comes back from SPI_slave
    //uint8_t from_SPI_slave =  SPI_readByte();

    // Print out what was sent back from Slave SPI
    //Serial.print("Reply: ");
    //Serial.println(from_SPI_slave);
    
    //delay(1500);
    //Serial.println("Write SPI_LED_OFF");
    //SPI_writeByte(2);
  //}



} // loop()
